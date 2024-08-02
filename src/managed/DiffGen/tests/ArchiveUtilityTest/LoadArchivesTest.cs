/**
 * @file RecipeTest.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtilityTest;

using System;
using System.IO;

using ArchiveUtility;
using CpioArchives;
using Ext4Archives;
using Microsoft.Extensions.Logging;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using SWUpdateArchives;
using TarArchives;

[TestClass]
public class LoadArchivesTest
{
    private string TempFolder = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
    private ILogger Logger = InitLogger();

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

    private static ILogger InitLogger()
    {
        using ILoggerFactory loggerFactory =
            LoggerFactory.Create(builder =>
                builder.AddSimpleConsole(options =>
                {
                    options.IncludeScopes = false;
                    options.SingleLine = true;
                    options.TimestampFormat = "HH:mm:ss ";
                }));

        var logger = loggerFactory.CreateLogger<LoadArchivesTest>();

        ArchiveLoaderContext.DefaultLogger = logger;

        return logger;
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
            if (!Directory.Exists(TempFolder))
            {
                return;
            }

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

    [DataRow("samples/diffs/simple/source.cpio", "samples/diffs/simple/source.cpio.json", "cpio", "binary")]
    [DataRow("samples/diffs/complex/source.cpio", "samples/diffs/complex/source.cpio.json", "cpio", "binary")]
    [DataTestMethod]
    public void Test(string archiveFile, string expectedJsonFile, string archiveType, string archiveSubtype)
    {
        Assert.IsTrue(File.Exists(archiveFile));

        using var stream = File.OpenRead(archiveFile);

        ArchiveLoaderContext context = new(stream, TempFolder);

        bool result = ArchiveLoader.TryLoadArchive(context, out ArchiveTokenization tokens);
        Assert.IsTrue(result);

        Assert.AreEqual(archiveType, tokens.Type);
        Assert.AreEqual(archiveSubtype, tokens.Subtype);

        var actualJson = tokens.ToJson();

        var expectedJson = File.ReadAllText(expectedJsonFile);
        var actualJsonFile = Path.GetFullPath(expectedJsonFile + ".actual");
        File.WriteAllText(actualJsonFile, actualJson);

        Logger.LogInformation("Saving actual results to: {actualJsonFile}", actualJsonFile);

        Assert.AreEqual(expectedJson, actualJson);

        var tokensFromSaved = ArchiveTokenization.FromJsonPath(actualJsonFile);
        var fromSavedJson = tokensFromSaved.ToJson();

        Assert.AreEqual(expectedJson, fromSavedJson);
    }
}