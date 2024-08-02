/**
 * @file DiffTests.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace UnitTests;

using System;
using System.IO;

using ArchiveUtility;
using CpioArchives;
using Ext4Archives;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SWUpdateArchives;
using TarArchives;

[TestClass]
public class DiffTests
{
    private string TempFolder;

    [ClassInitialize]
    public static void ExpansionTestsInitialize(TestContext context)
    {
        ArchiveLoader.RegisterArchiveType(typeof(OldStyleTarArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(AsciiCpioArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(NewAsciiCpioArchive), 99);
        ArchiveLoader.RegisterArchiveType(typeof(BinaryCpioArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(SWUpdateArchive), 98);
        ArchiveLoader.RegisterArchiveType(typeof(Ext4Archive), 100);
    }

    [TestInitialize]
    public void TestInitialize()
    {
        TempFolder = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
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

    // [DataRow("samples/diffs/simple/source.cpio", "samples/diffs/simple/target.cpio", "samples/diffs/simple/logs")]
    // [DataRow("samples/diffs/nested/source.cpio", "samples/diffs/nested/target.cpio", "samples/diffs/nested/logs")]
    // [DataRow("samples/diffs/complex/source.cpio", "samples/diffs/complex/target.cpio", "samples/diffs/complex/logs")]
    // [DataTestMethod]
    // public void Test_DiffGeneration(string sourceFile, string targetFile, string expectedLogFolder)
    // {
        // if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        // {
            // //in the nested test case, the cpio inside the cpio has its remainder deflated differently in Windows and Linux.
            // if (expectedLogFolder.Contains("nested"))
            // {
                // expectedLogFolder += "_linux";
            // }
        // }
        // else
        // {
            // AddNativeCodeToPath();
        // }

        // string outputFile = Path.Combine(TempFolder, "output.file");
        // string logFolder = Path.Combine(TempFolder, "log");
        // string workingFolder = Path.Combine(TempFolder, "working");

        // if (Directory.Exists(logFolder))
        // {
            // Directory.Delete(logFolder);
        // }

        // if (Directory.Exists(workingFolder))
        // {
            // Directory.Delete(workingFolder);
        // }

        // DiffBuilder.Execute(new DiffBuilder.Parameters()
        // {
            // SourceFile = sourceFile,
            // TargetFile = targetFile,
            // OutputFile = outputFile,
            // LogFolder = logFolder,
            // WorkingFolder = workingFolder,
            // KeepWorkingFolder = true
        // });

        // TestUtility.DumpFolderHash(expectedLogFolder);

        // string diffFileName = "diff.json";
        // var expectedFiles = Directory.GetFiles(expectedLogFolder, diffFileName, SearchOption.AllDirectories).OrderBy(f => f).ToArray();
        // var actualFiles = Directory.GetFiles(logFolder, diffFileName, SearchOption.AllDirectories).OrderBy(f => f).ToArray();

        // Assert.AreEqual(expectedFiles.Length, actualFiles.Length);
        // for (int i = 0; i < expectedFiles.Length; i++)
        // {
            // string expectedFile = expectedFiles[i];
            // string actualFile = actualFiles[i];

            // var expectedFileReplaced = expectedFile.Replace(expectedLogFolder, string.Empty);
            // var actualFileReplaced = actualFile.Replace(logFolder, string.Empty);

            // TestUtility.CompareMultilineStrings(expectedFileReplaced, actualFileReplaced);

            // string expectedJson = File.ReadAllText(expectedFile);
            // string actualJson = File.ReadAllText(actualFile);

            // if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            // {
                // expectedJson = expectedJson
                    // .Replace("\r\n", "\n")
                    // .Replace("\\\\", "/");
            // }

            // // Because the expected and actual working folders are different paths, need to remove all working folder string
            // // instances from the json produced in the test code
            // string doubleBackslashedWorkingFolder = workingFolder.Replace(@"\", @"\\");
            // actualJson = actualJson.Replace(doubleBackslashedWorkingFolder, string.Empty);

            // var differences = TestUtility.GetLineDifferences(expectedJson, actualJson);

            // if (differences.Count > 0)
            // {
                // Console.WriteLine($"Differences detected between expecter: {expectedFile} and actual: {actualFile}");

                // for (int diffIndex = 0; diffIndex < differences.Count; diffIndex++)
                // {
                    // Console.WriteLine($"{diffIndex})");
                    // Console.WriteLine($"\tExpected: {differences[diffIndex].Expected}");
                    // Console.WriteLine($"\tActual  : {differences[diffIndex].Actual}");
                // }
            // }

            // Assert.AreEqual(0, differences.Count, $"Files are different. Expected: {expectedFile}, Actual: {actualFile}");
        // }
    // }
}
