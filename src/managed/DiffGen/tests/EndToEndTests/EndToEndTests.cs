/**
 * @file EndToEndTests.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace EndToEndTests;

using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using Microsoft.VisualStudio.TestPlatform.CoreUtilities.Extensions;
using Microsoft.VisualStudio.TestTools.UnitTesting;

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

    public static string RunningDirectory { get => Path.GetDirectoryName(typeof(EndToEndTests).Assembly.Location); }

    private static int LaunchProcess(string name, string arguments)
    {
        Console.WriteLine($"name: {name}, arguments: {arguments}");
        Process process = new();

        process.StartInfo.CreateNoWindow = true;
        process.StartInfo.RedirectStandardOutput = true;
        process.StartInfo.RedirectStandardError = true;
        process.StartInfo.UseShellExecute = false;

        process.StartInfo.FileName = name;
        process.StartInfo.Arguments = arguments;

        process.ErrorDataReceived += (_, data) =>
        {
            if (!string.IsNullOrEmpty(data.Data))
            {
                Console.WriteLine(data.Data);
            }
        };

        process.OutputDataReceived += (_, data) =>
        {
            if (!string.IsNullOrEmpty(data.Data))
            {
                Console.WriteLine(data.Data);
            }
        };

        process.Start();
        process.BeginErrorReadLine();
        process.BeginOutputReadLine();
        process.WaitForExit();

        return process.ExitCode;
    }

    private static byte[] GetFileHash(string path)
    {
        HashAlgorithm hash = SHA256.Create();

        using var stream = File.OpenRead(path);

        return hash.ComputeHash(stream);
    }

    private static void RunTest(string tempFolder, string sourceFile, string targetFile, int maxDiffSize)
    {
        var logFolder = Path.Combine(tempFolder, "logs");
        var workingFolder = Path.Combine(tempFolder, "working");
        var diffFile = Path.Combine(tempFolder, "output.diff");

        var sourceSize = new FileInfo(sourceFile).Length;
        var targetSize = new FileInfo(targetFile).Length;
        Trace.TraceInformation("Source Size: {0}", sourceSize);
        Trace.TraceInformation("Target Size: {0}", targetSize);

        string allArguments = $"{sourceFile} {targetFile} {diffFile} {logFolder} {workingFolder}";

        int ret = LaunchProcess("bin/DiffGenTool", allArguments);
        Assert.AreEqual(0, ret);
        Assert.IsTrue(File.Exists(diffFile));

        var diffSize = new FileInfo(diffFile).Length;
        Trace.TraceInformation("Diff Size: {0}", diffSize);

        Assert.IsTrue(diffSize < targetSize);
        Assert.IsTrue(diffSize < maxDiffSize);

        Console.WriteLine("Trying to apply diff.");

        var appliedDiffFile = Path.Combine(tempFolder, $"diff.applied");
        ret = LaunchProcess("bin/applydiff", $"{sourceFile} {diffFile} {appliedDiffFile}");
        Assert.AreEqual(0, ret);

        Assert.IsTrue(File.Exists(appliedDiffFile));

        var appliedHash = GetFileHash(appliedDiffFile);

        var targetHash = GetFileHash(targetFile);
        Assert.IsTrue(targetHash.SequenceEqual(appliedHash));
    }

    [DataRow("simple", "samples/diffs/simple/source.cpio", "samples/diffs/simple/target.cpio", 1_300_000)]
    [DataRow("complex", "samples/diffs/complex/source.cpio", "samples/diffs/complex/target.cpio", 850_000)]
    [DataRow("nested", "samples/diffs/swu/source/source.cpio", "samples/diffs/swu/target/target.cpio", 850_000)]
    [DataTestMethod]
    public void Test_DiffGeneration(string name, string sourceFile, string targetFile, int maxDiffSize)
    {
        var baseTestFolder = Path.Combine(TempFolder, "Test_DiffGeneration", name);
        RunTest(baseTestFolder, Path.GetFullPath(sourceFile), Path.GetFullPath(targetFile), maxDiffSize);
    }

    [DataRow("swu-recompressed", "samples/diffs/swu/source.swu", "samples/diffs/swu/target.swu", 850_000)]
    [DataTestMethod]
    public void Test_DiffGeneration_Recompress(string name, string sourceFile, string targetFile, int maxDiffSize)
    {
        var baseTestFolder = Path.Combine(TempFolder, "Test_DiffGeneration_Recompressed", name);
        var recompressedTargetFile = Path.Combine(baseTestFolder, "target.recompressed");

        Process recompress = new();
        recompress.StartInfo.FileName = "bin/recompress";
        recompress.StartInfo.Arguments = $"swu {targetFile} {recompressedTargetFile}";
        recompress.Start();
        recompress.WaitForExit();

        Assert.AreEqual(0, recompress.ExitCode);

        RunTest(baseTestFolder, sourceFile, recompressedTargetFile, maxDiffSize);
    }
}
