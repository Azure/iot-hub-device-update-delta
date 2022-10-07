/**
 * @file AddRemainderChunks.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Text.Json;
    using System.Threading;
    using Microsoft.Extensions.Logging;

    using ArchiveUtility;

    class AddRemainderChunks : Worker
    {
        public Diff Diff { get; set; }
        public string TargetFile { get; set; }

        public AddRemainderChunks(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        void WriteRemainderChunks(List<ArchiveItem> remainingChunks)
        {
            var largeChunks = remainingChunks.Where(c => c.Length > (1024 * 1024) && !c.Name.Contains("Gap")).ToList();

            string allPath = Path.Combine(WorkingFolder, "AllRemainderChunks.json");
            string largePath = Path.Combine(WorkingFolder, "LargeRemainderChunks.json");

            WriteRemainderChunksToPath(remainingChunks, allPath);
            WriteRemainderChunksToPath(largeChunks, largePath);
        }

        void WriteRemainderChunksToPath(List<ArchiveItem> remainingChunks, string path)
        {
            var options = ArchiveTokenization.GetStandardJsonSerializerOptions();
            options.WriteIndented = true;

            using var stream = File.Create(path);
            using var writer = new Utf8JsonWriter(stream, new JsonWriterOptions() { Indented = true });

            JsonSerializer.Serialize(writer, remainingChunks, options);
        }

        protected override void ExecuteInternal()
        {
            CheckForCancellation();

            Logger.LogInformation("Determining remaining chunks.");

            var remainingChunks = Diff.Chunks.Where(c => c.Recipes.Count == 0).ToList();

            WriteRemainderChunks(remainingChunks);

            Logger.LogInformation($"Remainder data will be used by {remainingChunks.Count} chunks.");

            string remainderPath = Path.Combine(WorkingFolder, "remainder.dat");
            string deflatedRemainderPath = remainderPath + ".deflate";

            Logger.LogInformation("Creating Remainder Blob: {0}", remainderPath);

            Stopwatch stopWatch = new();
            stopWatch.Start();

            // Build up the remainder, and while enumerating add recipes for
            // each chunk referring to the remainder.
            using (var archiveStream = File.OpenRead(TargetFile))
            {
                using (var remainderStream = File.Create(remainderPath))
                {
                    foreach (var chunk in remainingChunks)
                    {
                        CheckForCancellation();

                        chunk.Recipes.Add(new RemainderRecipe());

                        using (var substream = new SubStream(archiveStream, (long)chunk.Offset, (long)chunk.Length))
                        {
                            substream.CopyTo(remainderStream);
                        }
                    }
                }
            }

            stopWatch.Stop();
            var elapsed = stopWatch.Elapsed;
            Logger.LogInformation($"Building remainder finished at {DateTime.Now}. Elapsed Time: {elapsed} ");

            FileInfo remainderFileInfo = new(remainderPath);
            Logger.LogInformation($"{remainderPath} is {remainderFileInfo.Length:#,0.####} bytes.");
            var remainderHash = Hash.Sha256FileAsString(remainderPath);
            Logger.LogInformation("Remainder Sha256 Hash: {0}", remainderHash);

            Logger.LogInformation($"Deflating file: {remainderPath}, deflateFile: {deflatedRemainderPath}");
            stopWatch.Reset();
            stopWatch.Start();
            DeflateFile(remainderPath, deflatedRemainderPath);
            stopWatch.Stop();
            elapsed = stopWatch.Elapsed;
            Logger.LogInformation($"Deflating remainder finished at {DateTime.Now}. Elapsed Time: {elapsed} ");

            FileInfo deflateFileInfo = new(deflatedRemainderPath);
            Logger.LogInformation($"{deflatedRemainderPath} is {deflateFileInfo.Length:#,0.####} bytes.");
            var deflatedRemainderHash = Hash.Sha256FileAsString(deflatedRemainderPath);
            Logger.LogInformation("Deflated Remainder Sha256 Hash: {0}", deflatedRemainderHash);

            Diff.RemainderUncompressedSize = (ulong)remainderFileInfo.Length;
            Diff.RemainderCompressedSize = (ulong)deflateFileInfo.Length;
            Diff.RemainderPath = deflatedRemainderPath;
        }

        private static void DeflateFile(string file, string output)
        {
            using (var readStream = File.OpenRead(file))
            {
                using (var writeStream = File.Create(output))
                {
                    using (var compressStream = new DeflateStream(writeStream, CompressionMode.Compress))
                    {
                        readStream.CopyTo(compressStream);
                    }
                }
            }
        }
    }
}

