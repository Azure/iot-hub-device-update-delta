/**
 * @file Program.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.IO;
using ArchiveUtility;
using CpioArchives;
using TarArchives;
using SWUpdateArchives;
using Ext4Archives;

namespace CpioHandler
{
    class Program
    {
        static void Demo(string path)
        {
            ArchiveTokenization tokens;

            using (var readStream = File.OpenRead(path))
            {
                if (!ArchiveLoader.TryLoadArchive(readStream, Path.GetTempPath(), out tokens, "cpio"))
                {
                    Console.WriteLine("{0} is not a cpio archive.", path);
                }

                if (!ArchiveLoader.TryLoadArchive(readStream, Path.GetTempPath(), out tokens))
                {
                    Console.WriteLine("{0} is not a supported archive.", path);
                    return;
                }
            }

            Console.WriteLine("{0} is a {1} ({2}) archive.", path, tokens.Type, tokens.Subtype);
            var jsonFileName = Path.GetFileName(path) + ".json";
            var jsonPath = Path.Combine(Path.GetTempPath(), jsonFileName);
            Console.WriteLine("Writing Json to {0}", jsonPath);
            using (var writeStream = File.Open(jsonPath, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                tokens.WriteJson(writeStream, true);
            }
        }

        static void Main(string[] args)
        {
            ArchiveLoader.RegisterArchiveType(typeof(OldStyleTarArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(AsciiCpioArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(NewAsciiCpioArchive), 99);
            ArchiveLoader.RegisterArchiveType(typeof(BinaryCpioArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(SWUpdateArchive), 98);
            ArchiveLoader.RegisterArchiveType(typeof(Ext4Archive), 100);

            if (args.Length != 1)
            {
                Usage();
                return;
            }

            Demo(args[0]);
        }

        static void Usage()
        {
            Console.WriteLine("ArchiveLoaderDemo.exe <archive path>");
        }
    }
}
