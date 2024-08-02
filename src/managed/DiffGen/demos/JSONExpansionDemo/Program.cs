/**
 * @file Program.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace JSONExpansionDemo;

using System;
using System.IO;

using ArchiveUtility;
using CpioArchives;
using Ext4Archives;
using Microsoft.Extensions.Logging;
using SWUpdateArchives;
using TarArchives;

public class Program
{
    public static void Main(string[] args)
    {
        using ILoggerFactory loggerFactory =
            LoggerFactory.Create(builder =>
                builder.AddSimpleConsole(options =>
                {
                    options.IncludeScopes = false;
                    options.SingleLine = true;
                    options.TimestampFormat = "HH:mm:ss ";
                }));

        var logger = loggerFactory.CreateLogger<Program>();

        ArchiveLoader.RegisterArchiveType(typeof(OldStyleTarArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(AsciiCpioArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(NewAsciiCpioArchive), 99);
        ArchiveLoader.RegisterArchiveType(typeof(BinaryCpioArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(SWUpdateArchive), 98);
        ArchiveLoader.RegisterArchiveType(typeof(Ext4Archive), 100);

        ArchiveLoaderContext.DefaultLogger = logger;

        if (args.Length == 0)
        {
            Usage(logger);
            return;
        }

        switch (args[0])
        {
            case "fillgaps":
                FillGapsCmd(logger, args);
                return;
            case "tokenize":
                TokenizeCmd(logger, args);
                return;
            case "load_tokens":
                LoadTokensCmd(logger, args);
                return;
            default:
                Usage(logger);
                return;
        }
    }

    //this demo reads a JSON manifest and the file it describes,
    //fills any gaps in the JSON and writes it to a new file
    private static void FillGaps(ILogger logger, string archiveFile, string jsonPath, string newJsonPath)
    {
        //read the JSON manifest
        ArchiveTokenization tokens;
        using (var readStream = File.OpenRead(jsonPath))
        {
            tokens = ArchiveTokenization.FromJson(readStream);
        }

        //extrapolate its gaps and calculate their hashes
        using (var readStream = File.OpenRead(archiveFile))
        {
            tokens.HandleGapChunks(readStream);
        }

        //write the new JSON
        using (var writeStream = File.Open(newJsonPath, FileMode.Create, FileAccess.Write, FileShare.None))
        {
            tokens.WriteJson(writeStream, true);
        }
    }

    private static void FillGapsCmd(ILogger logger, string[] args)
    {
        if (args.Length != 4)
        {
            FillGapsUsage(logger);
            return;
        }

        FillGaps(logger, args[1], args[2], args[3]);
    }

    private static void FillGapsUsage(ILogger logger)
    {
        logger.LogInformation("JSONExpansionDemo.exe fillgaps <original archive path> <existing JSON manifest path> <updated JSON manifest path>");
    }

    // this command tokenizes a given archiveFile and saves it to the output path
    private static void Tokenize(ILogger logger, string archiveFile, string jsonPath, bool indent)
    {
        using var stream = File.OpenRead(archiveFile);

        var workingDirectory = Directory.CreateTempSubdirectory();
        var workingPath = workingDirectory.FullName;

        ArchiveLoaderContext context = new(stream, workingPath);

        logger.LogInformation("Loading archive at: {archiveFile}", archiveFile);
        if (ArchiveLoader.TryLoadArchive(context, out ArchiveTokenization tokens))
        {
            var json = tokens.ToJson(indent);
            logger.LogInformation("Writing tokens to {jsonFile}", jsonPath);
            File.WriteAllText(jsonPath, json);
        }
        else
        {
            logger.LogError("Failed to tokenize archive: {archiveFile}", archiveFile);
        }

        workingDirectory.Delete(true);
    }

    private static void TokenizeCmd(ILogger logger, string[] args)
    {
        if (args.Length != 4)
        {
            TokenizeUsage(logger);
            return;
        }

        bool indent;
        if (!bool.TryParse(args[3], out indent))
        {
            TokenizeUsage(logger);
            return;
        }

        Tokenize(logger, args[1], args[2], indent);
    }

    private static void TokenizeUsage(ILogger logger)
    {
        logger.LogInformation("JSONExpansionDemo.exe tokenize <original archive path> <json output path> <indent:boolean>");
    }

    //this demo reads a JSON manifest and the file it describes,
    //and writes each described payload into its own file in the provided folder
    private static void LoadTokens(ILogger logger, string jsonPath)
    {
        try
        {
            var tokens = ArchiveTokenization.FromJsonPath(jsonPath);
        }
        catch (Exception e)
        {
            logger.LogInformation("Failed to load json at path: {jsonPath}. Exception: {msg}", jsonPath, e.Message);
            return;
        }

        logger.LogInformation("Successfully loaded json at path: {jsonPath}", jsonPath);
    }

    private static void LoadTokensCmd(ILogger logger, string[] args)
    {
        if (args.Length != 2)
        {
            LoadTokensUsage(logger);
            return;
        }

        LoadTokens(logger, args[1]);
    }

    private static void LoadTokensUsage(ILogger logger)
    {
        logger.LogInformation("JSONExpansionDemo.exe load_tokens <JSON manifest path>");
    }

    private static void Usage(ILogger logger)
    {
        logger.LogInformation("JSONExpansionDemo.exe (fillgaps|tokenize|step4) args...");
    }
}
