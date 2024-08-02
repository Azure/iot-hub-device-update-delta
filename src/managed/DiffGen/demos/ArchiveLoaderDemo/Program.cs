/**
 * @file Program.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace CpioHandler;

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
        ArchiveLoader.RegisterArchiveType(typeof(OldStyleTarArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(AsciiCpioArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(NewAsciiCpioArchive), 99);
        ArchiveLoader.RegisterArchiveType(typeof(BinaryCpioArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(SWUpdateArchive), 98);
        ArchiveLoader.RegisterArchiveType(typeof(Ext4Archive), 100);

        string jsonPath;
        if (args.Length == 1)
        {
            var jsonFileName = Path.GetFileName(args[0]) + ".json";
            jsonPath = Path.Combine(Path.GetTempPath(), jsonFileName);
        }
        else if (args.Length == 2)
        {
            jsonPath = args[1];
        }
        else
        {
            Usage();
            return;
        }

        Demo(args[0], jsonPath);
    }

    private static void Demo(string path, string jsonPath)
    {
        ArchiveTokenization tokens;

        string logPath = Path.Combine(Path.GetTempPath(), "demoLogs");
        ILogger logger = new DiffLogger(logPath);

        using (var readStream = File.OpenRead(path))
        {
            var context = new ArchiveLoaderContext(readStream, Path.GetTempPath(), logger, LogLevel.Information);

            if (!ArchiveLoader.TryLoadArchive(context, out tokens, "cpio"))
            {
                Console.WriteLine("{0} is not a cpio archive.", path);
            }

            if (!ArchiveLoader.TryLoadArchive(context, out tokens))
            {
                Console.WriteLine("{0} is not a supported archive.", path);
                return;
            }
        }

        Console.WriteLine("{0} is a {1} ({2}) archive.", path, tokens.Type, tokens.Subtype);
        Console.WriteLine("Writing Json to {0}", jsonPath);
        using (var writeStream = File.Open(jsonPath, FileMode.Create, FileAccess.Write, FileShare.None))
        {
            tokens.WriteJson(writeStream, true);
        }
    }

    private static void Usage()
    {
        Console.WriteLine("ArchiveLoaderDemo.exe <archive path>");
    }
}
