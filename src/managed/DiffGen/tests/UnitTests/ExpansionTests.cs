/**
 * @file ExpansionTests.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

namespace UnitTests
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;

    using Microsoft.VisualStudio.TestTools.UnitTesting;

    using ArchiveUtility;
    using CpioArchives;
    using Ext4Archives;
    using SWUpdateArchives;
    using TarArchives;

    [TestClass]
    public class ExpansionTests
    {
        [ClassInitialize]
        public static void ExpansionTestsInitialize(TestContext context)
        {
            ArchiveLoader.RegisterArchiveType(typeof(OldStyleTarArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(AsciiCpioArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(NewAsciiCpioArchive), 99);
            ArchiveLoader.RegisterArchiveType(typeof(BinaryCpioArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(SWUpdateArchive), 98);
            ArchiveLoader.RegisterArchiveType(typeof(Ext4Archive), 97);
        }

        [DataRow("samples/archives/cpio1.swu", "samples/archive_manifests/cpio1.swu.json")]
        [DataRow("samples/archives/cpio2.cpio", "samples/archive_manifests/cpio2.cpio.json")]
        [DataRow("samples/archives/cpio3.cpio", "samples/archive_manifests/cpio3.cpio.json")]
        [DataRow("samples/archives/cpio4.cpio", "samples/archive_manifests/cpio4.cpio.json")]
        [DataRow("samples/archives/cpio5.cpio", "samples/archive_manifests/cpio5.cpio.json")]
        [DataRow("samples/archives/cpio6.cpio", "samples/archive_manifests/cpio6.cpio.json")]
        [DataRow("samples/archives/ext4/sample1.ext4", "samples/archive_manifests/ext4/sample1.ext4.json")]
        [DataRow("samples/archives/ext4/sample2.ext4", "samples/archive_manifests/ext4/sample2.ext4.json")]
        [DataRow("samples/archives/ext4/sample3.ext4", "samples/archive_manifests/ext4/sample3.ext4.json")]
        [DataTestMethod]
        public void Test_ArchiveTokenization(string archivePath, string expectedJsonManifestPath)
        {
            var runningDirectory = ProcessHelper.GetPathInRunningDirectory(String.Empty);

            Trace.TraceInformation($"Running directory: {runningDirectory}");

            string dumpextfsTool = "dumpextfs";

            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                dumpextfsTool = dumpextfsTool + ".exe";
            }

            var dumpextfsPath = ProcessHelper.GetPathInRunningDirectory(dumpextfsTool);
            Assert.IsTrue(File.Exists(dumpextfsPath));

            ArchiveTokenization tokens;
            using (var readStream = File.OpenRead(archivePath))
            {
                if (!ArchiveLoader.TryLoadArchive(readStream, Path.GetTempPath(), out tokens))
                {
                    Assert.Fail();
                }
            }

            string actualJson = tokens.ToJson(true);
            string expectedJson = File.ReadAllText(expectedJsonManifestPath);

            TestUtility.CompareMultilineStrings(expectedJson, actualJson);
        }

        [DataRow("samples/archives/cpio1.swu", "samples/archive_manifests/cpio1.swu.json", "samples/expanded_chunks/cpio1")]
        [DataRow("samples/archives/cpio2.cpio", "samples/archive_manifests/cpio2.cpio.json", "samples/expanded_chunks/cpio2")]
        [DataRow("samples/archives/cpio3.cpio", "samples/archive_manifests/cpio3.cpio.json", "samples/expanded_chunks/cpio3")]
        [DataRow("samples/archives/cpio4.cpio", "samples/archive_manifests/cpio4.cpio.json", "samples/expanded_chunks/cpio4")]
        [DataRow("samples/archives/cpio5.cpio", "samples/archive_manifests/cpio5.cpio.json", "samples/expanded_chunks/cpio5")]
        [DataRow("samples/archives/cpio6.cpio", "samples/archive_manifests/cpio6.cpio.json", "samples/expanded_chunks/cpio6")]
        [DataRow("samples/archives/ext4/sample1.ext4", "samples/archive_manifests/ext4/sample1.ext4.json", "samples/expanded_chunks/ext4_1")]
        [DataRow("samples/archives/ext4/sample2.ext4", "samples/archive_manifests/ext4/sample2.ext4.json", "samples/expanded_chunks/ext4_2")]
        [DataRow("samples/archives/ext4/sample3.ext4", "samples/archive_manifests/ext4/sample3.ext4.json", "samples/expanded_chunks/ext4_3")]
        [DataTestMethod]
        public void Test_ChunkExtraction(string archivePath, string jsonManifestPath, string expectedFolderContent)
        {
            //read the JSON manifest
            ArchiveTokenization tokens;
            using (var readStream = File.OpenRead(jsonManifestPath))
            {
                tokens = ArchiveTokenization.FromJson(readStream);
            }

            //create a temporary folder for the output
            var outputFolderPath = Path.Combine(Path.GetTempPath(), "expanded_chunks");
            if (Directory.Exists(outputFolderPath))
            {
                Directory.Delete(outputFolderPath, true);
            }

            //expand the archive into individual chunk files
            using (var readStream = File.OpenRead(archivePath))
            {
                ChunkExtractor.ExtractChunks(tokens, readStream, outputFolderPath);
            }
            ChunkExtractor.ValidateExtractedChunks(tokens, outputFolderPath);

            //compare the created folder with the ground truth folder
            AssertFoldersAreEqual(expectedFolderContent, outputFolderPath);

            Directory.Delete(outputFolderPath, true);
        }

        [DataRow("samples/archives/cpio1.swu", "samples/archive_manifests/cpio1.swu.json", "samples/expanded_payloads/cpio1")]
        [DataRow("samples/archives/cpio2.cpio", "samples/archive_manifests/cpio2.cpio.json", "samples/expanded_payloads/cpio2")]
        [DataRow("samples/archives/cpio3.cpio", "samples/archive_manifests/cpio3.cpio.json", "samples/expanded_payloads/cpio3")]
        [DataRow("samples/archives/cpio4.cpio", "samples/archive_manifests/cpio4.cpio.json", "samples/expanded_payloads/cpio4")]
        [DataRow("samples/archives/cpio5.cpio", "samples/archive_manifests/cpio5.cpio.json", "samples/expanded_payloads/cpio5")]
        [DataRow("samples/archives/cpio6.cpio", "samples/archive_manifests/cpio6.cpio.json", "samples/expanded_payloads/cpio6")]
        [DataRow("samples/archives/ext4/sample1.ext4", "samples/archive_manifests/ext4/sample1.ext4.json", "samples/expanded_payloads/ext4_1")]
        [DataRow("samples/archives/ext4/sample2.ext4", "samples/archive_manifests/ext4/sample2.ext4.json", "samples/expanded_payloads/ext4_2")]
        [DataRow("samples/archives/ext4/sample3.ext4", "samples/archive_manifests/ext4/sample3.ext4.json", "samples/expanded_payloads/ext4_3")]
        [DataTestMethod]
        public void Test_PayloadExtraction(string archivePath, string jsonManifestPath, string expectedFolderContent)
        {
            //read the JSON manifest
            ArchiveTokenization tokens;
            using (var readStream = File.OpenRead(jsonManifestPath))
            {
                tokens = ArchiveTokenization.FromJson(readStream);
            }

            //create a temporary folder for the output
            var outputFolderPath = Path.Combine(Path.GetTempPath(), "expanded_payloads");
            if (Directory.Exists(outputFolderPath))
            {
                Directory.Delete(outputFolderPath, true);
            }

            //expand the archive into individual chunk files
            using (var readStream = File.OpenRead(archivePath))
            {
                PayloadExtractor.ExtractPayload(tokens, readStream, outputFolderPath);
            }
            PayloadExtractor.ValidateExtractedPayload(tokens, outputFolderPath);

            //compare the created folder with the ground truth folder
            AssertFoldersAreEqual(expectedFolderContent, outputFolderPath);

            Directory.Delete(outputFolderPath, true);
        }

        private static void AssertFoldersAreEqual(string expectedFolderContent, string actualFolderContent)
        {
            var expectedFilePaths = Directory.EnumerateFiles(expectedFolderContent, "*", SearchOption.AllDirectories).ToList();
            var actualFilePaths = Directory.EnumerateFiles(actualFolderContent, "*", SearchOption.AllDirectories).ToList();

            expectedFilePaths.Sort();
            actualFilePaths.Sort();

            Assert.AreEqual(expectedFilePaths.Count, actualFilePaths.Count);

            foreach (var filePair in expectedFilePaths.Zip(actualFilePaths))
            {
                var expectedFile = filePair.First;
                var actualFile = filePair.Second;
                Assert.AreEqual(Path.GetFileName(expectedFile), Path.GetFileName(actualFile));
                AssertFilesAreEqual(expectedFile, actualFile);
            }
        }

        private static void AssertFilesAreEqual(string expectedFilePath, string actualFilePath)
        {
            var expectedFileData = File.ReadAllBytes(expectedFilePath);
            var actualFileData = File.ReadAllBytes(actualFilePath);
            var expectedBase64 = Convert.ToBase64String(expectedFileData);
            var actualBase64 = Convert.ToBase64String(actualFileData);

            // Comparing like this gives us something we can debug if we just have a log
            // and no file system to look at.
            Assert.AreEqual(expectedBase64, actualBase64);
        }
    }
}
