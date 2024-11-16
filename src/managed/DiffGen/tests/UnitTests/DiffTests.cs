/**
 * @file DiffTests.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace UnitTests;

using System;
using System.Diagnostics;
using System.IO;
using System.Linq;

using ArchiveUtility;
using CpioArchives;
using Ext4Archives;
using Microsoft.Azure.DeviceUpdate.Diffs;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
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

    [DataRow("samples/diffs/simple/source.cpio", "samples/diffs/simple/target.cpio")]
    [DataRow("samples/diffs/nested/source.cpio", "samples/diffs/nested/target.cpio")]
    [DataRow("samples/diffs/complex/source.cpio", "samples/diffs/complex/target.cpio")]
    [DataTestMethod]
    public void Test_DiffGeneration(string sourceFile, string targetFile)
    {
        string outputFile = Path.Combine(TempFolder, "output.file");
        string logFolder = Path.Combine(TempFolder, "log");
        string workingFolder = Path.Combine(TempFolder, "working");

        if (Directory.Exists(logFolder))
        {
            Directory.Delete(logFolder);
        }

        if (Directory.Exists(workingFolder))
        {
            Directory.Delete(workingFolder);
        }

        DiffBuilder.Execute(new DiffBuilder.Parameters()
        {
            SourceFile = sourceFile,
            TargetFile = targetFile,
            OutputFile = outputFile,
            LogFolder = logFolder,
            WorkingFolder = workingFolder,
            KeepWorkingFolder = true,
        });
    }

    [DataRow("samples/diffs/swu/source.swu", "samples/diffs/swu/target.swu")]
    [DataTestMethod]
    public void Test_DiffGeneration_Undiffable(string sourceFile, string targetFile)
    {
        string outputFile = Path.Combine(TempFolder, "output.file");
        string logFolder = Path.Combine(TempFolder, "log");
        string workingFolder = Path.Combine(TempFolder, "working");

        if (Directory.Exists(logFolder))
        {
            Directory.Delete(logFolder);
        }

        if (Directory.Exists(workingFolder))
        {
            Directory.Delete(workingFolder);
        }

        bool caughtException = false;
        try
        {
            DiffBuilder.Execute(new DiffBuilder.Parameters()
            {
                SourceFile = sourceFile,
                TargetFile = targetFile,
                OutputFile = outputFile,
                LogFolder = logFolder,
                WorkingFolder = workingFolder,
                KeepWorkingFolder = true,
            });
        }
        catch (DiffBuilderException e)
        {
            caughtException = e.FailureType == FailureType.UndiffableTarget;
        }

        Assert.IsTrue(caughtException);
    }
}
