/**
 * @file EndToEndTests.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection.Metadata;
using System.Runtime.InteropServices;
using System.Runtime.Intrinsics.Arm;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Threading.Tasks;

using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTests
{
    [TestClass]
    public class EndToEndTests
    {
        private string TempFolder;

        [ClassInitialize]
        public static void ExpansionTestsInitialize(TestContext context)
        {
        }

        [TestInitialize]
        public void TestInitialize()
        {
            TempFolder = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
            Directory.CreateDirectory(TempFolder);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            // Need below garbage collection calls so files in TempFolder are not being used by another process
            GC.Collect();
            GC.WaitForPendingFinalizers();

            // Implementing retry loop to attempt bypassing unknown error that sometimes occurs in pipeline
            const int max_retries = 5;
            int retries = 0;
            while (true)
            {
                try
                {
                    Directory.Delete(TempFolder, true /* recursive */);
                }
                catch (IOException)
                {
                    if (retries++ < max_retries)
                    {
                        System.Threading.Thread.Sleep(1000);
                        continue;
                    }
                    throw;
                }

                break;
            }
        }

        static string RunningDirectory { get => Path.GetDirectoryName(typeof(EndToEndTests).Assembly.Location); }

        int LaunchProcess(string name, string arguments)
        {
            Process process = new();
            process.StartInfo.FileName = name;
            process.StartInfo.Arguments = arguments;

            process.Start();

            process.WaitForExit();

            return process.ExitCode;
        }

        byte[] GetFileHash(string path)
        {
            HashAlgorithm hash = SHA256.Create();

            using var stream = File.OpenRead(path);

            return hash.ComputeHash(stream);
        }

        void RunTest(string tempFolder, string sourceFile, string targetFile, string recompressedTargetFile, int maxDiffSize)
        {
            var logFolder = Path.Combine(tempFolder, "logs");
            var workingFolder = Path.Combine(tempFolder, "working");
            var diffFile = Path.Combine(tempFolder, "output.diff");

            var sourceSize = new FileInfo(sourceFile).Length;
            var targetSize = new FileInfo(targetFile).Length;
            Trace.TraceInformation("Source Size: {0}", sourceSize);
            Trace.TraceInformation("Target Size: {0}", targetSize);

            string allArguments = $"{sourceFile} {targetFile} {diffFile} {logFolder} {workingFolder} {recompressedTargetFile}";

            int ret = LaunchProcess("DiffGenTool", allArguments);
            Assert.AreEqual(0, ret);
            Assert.IsTrue(File.Exists(diffFile));

            var diffSize = new FileInfo(diffFile).Length;
            Trace.TraceInformation("Diff Size: {0}", diffSize);

            Assert.IsTrue(diffSize < targetSize);
            Assert.IsTrue(diffSize < maxDiffSize);

            if (!string.IsNullOrEmpty(recompressedTargetFile))
            {
                Assert.IsTrue(File.Exists(recompressedTargetFile));
            }

            var appliedDiffFile = Path.Combine(tempFolder, $"diff.applied");
            ret = LaunchProcess("applydiff", $"{sourceFile} {diffFile} {appliedDiffFile}");
            Assert.AreEqual(0, ret);

            Assert.IsTrue(File.Exists(appliedDiffFile));

            var appliedHash = GetFileHash(appliedDiffFile);

            if (string.IsNullOrEmpty(recompressedTargetFile))
            {
                var targetHash = GetFileHash(targetFile);
                Assert.IsTrue(targetHash.SequenceEqual(appliedHash));
            }
            else
            {
                var recompressedTargetHash = GetFileHash(recompressedTargetFile);
                Assert.IsTrue(recompressedTargetHash.SequenceEqual(appliedHash));
            }
        }

        [DataRow("simple", "samples/diffs/simple/source.cpio", "samples/diffs/simple/target.cpio", 1_500_000)]
        [DataRow("nested", "samples/diffs/nested/source.cpio", "samples/diffs/nested/target.cpio", 4_500_000)]
        [DataRow("complex", "samples/diffs/complex/source.cpio", "samples/diffs/complex/target.cpio", 900_000)]
        [DataRow("swu", "samples/diffs/swu/source.swu", "samples/diffs/swu/target.swu", 110_000_000)]
        [DataRow("swu-recompressed", "samples/diffs/swu/source.swu", "samples/diffs/swu/target-recompressed.swu", 3_500_000)]
        [DataTestMethod]
        public void Test_DiffGeneration(string name, string sourceFile, string targetFile, int maxDiffSize)
        {
            var baseTestFolder = Path.Combine(TempFolder, "Test_DiffGeneration", name);
            RunTest(baseTestFolder, sourceFile, targetFile, "", maxDiffSize);
        }

        [DataRow("swu-recompressed", "samples/diffs/swu/source.swu", "samples/diffs/swu/target.swu", 3_500_000)]
        [DataTestMethod]
        public void Test_DiffGeneration_Recompress(string name, string sourceFile, string targetFile, int maxDiffSize)
        {
            if (!RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                return;
            }

            var baseTestFolder = Path.Combine(TempFolder, "Test_DiffGeneration_Recompressed", name);
            var recompressedTargetFile = Path.Combine(baseTestFolder, "target.recompressed");

            RunTest(baseTestFolder, sourceFile, targetFile, recompressedTargetFile, maxDiffSize);
        }
    }
}
