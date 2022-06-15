/**
 * @file Program.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using ArchiveUtility;
using System;
using System.IO;

namespace JSONExpansionDemo
{
    class Program
    {
        //this demo reads a JSON manifest and the file it describes,
        //fills any gaps in the JSON and writes it to a new file
        static void Step2Demo(string archiveFile, string jsonPath, string newJsonPath)
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
                tokens.FillChunkGaps(readStream);
            }

            //write the new JSON
            using (var writeStream = File.Open(newJsonPath, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                tokens.WriteJson(writeStream, true);
            }
        }

        static void Step2(string[] args)
        {
            if (args.Length != 4)
            {
                Step2Usage();
                return;
            }

            Step2Demo(args[1], args[2], args[3]);
        }

        static void Step2Usage()
        {
            Console.WriteLine("JSONExpansionDemo.exe step2 <original archive path> <existing JSON manifest path> <updated JSON manifest path>");
        }

        //this demo reads a JSON manifest and the file it describes,
        //and writes each described chunk into its own file in the provided folder
        static void Step3Demo(string archiveFile, string jsonPath, string outputFolderPath)
        {
            //read the JSON manifest
            ArchiveTokenization tokens;
            using (var readStream = File.OpenRead(jsonPath))
            {
                tokens = ArchiveTokenization.FromJson(readStream);
            }

            //then break down the archive file into its chunks
            using (var readStream = File.OpenRead(archiveFile))
            {
                ChunkExtractor.ExtractChunks(tokens, readStream, outputFolderPath);
            }
            ChunkExtractor.ValidateExtractedChunks(tokens, outputFolderPath);
        }

        //this demo reads a JSON manifest and the file it describes,
        //and writes each described payload into its own file in the provided folder
        static void Step4Demo(string archiveFile, string jsonPath, string outputFolderPath)
        {
            //read the JSON manifest
            ArchiveTokenization tokens;
            using (var readStream = File.OpenRead(jsonPath))
            {
                tokens = ArchiveTokenization.FromJson(readStream);
            }

            //then break down the archive file into its chunks
            using (var readStream = File.OpenRead(archiveFile))
            {
                PayloadExtractor.ExtractPayload(tokens, readStream, outputFolderPath);
            }
            PayloadExtractor.ValidateExtractedPayload(tokens, outputFolderPath);
        }

        static void Step3(string[] args)
        {
            if (args.Length != 4)
            {
                Step3Usage();
                return;
            }

            Step3Demo(args[1], args[2], args[3]);
        }

        static void Step4(string[] args)
        {
            if (args.Length != 4)
            {
                Step4Usage();
                return;
            }

            Step4Demo(args[1], args[2], args[3]);
        }

        static void Step3Usage()
        {
            Console.WriteLine("JSONExpansionDemo.exe step3 <original archive path> <JSON manifest path> <output folder path>");
        }

        static void Step4Usage()
        {
            Console.WriteLine("JSONExpansionDemo.exe step4 <original archive path> <JSON manifest path> <output folder path>");
        }

        private static void Usage()
        {
            Console.WriteLine("JSONExpansionDemo.exe (step2|step3|step4) args...");
        }

        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Usage();
                return;
            }
            switch (args[0])
            {
                case "step2":
                    Step2(args);
                    return;
                case "step3":
                    Step3(args);
                    return;
                case "step4":
                    Step4(args);
                    return;
                default:
                    Usage();
                    return;
            }
        }
    }
}
