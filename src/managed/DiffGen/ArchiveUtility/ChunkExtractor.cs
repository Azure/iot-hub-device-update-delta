/**
 * @file ChunkExtractor.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.IO;
    using System.Linq;

    public class ChunkExtractor
    {
        public static void ExtractChunk(ArchiveItem chunk, Stream archiveFile, string path)
        {
            archiveFile.Seek((long)chunk.Offset.Value, SeekOrigin.Begin);

            string parentPath = Path.GetDirectoryName(path);
            Directory.CreateDirectory(parentPath);

            using (var writeStream = File.Open(path, FileMode.Create, FileAccess.Write, FileShare.None))
            {
                using (var subStream = new SubStream(archiveFile, (long)chunk.Offset.Value, (long)chunk.Length))
                {
                    subStream.CopyTo(writeStream);
                }
            }
        }

        public static void ExtractChunks(ArchiveTokenization tokens, Stream archiveFile, string outputFolderPath)
        {
            if (!Directory.Exists(outputFolderPath))
            {
                Directory.CreateDirectory(outputFolderPath);
            }

            foreach (var chunk in tokens.Chunks.Where(c => !c.HasAllZeroRecipe()))
            {
                string chunkFilePath = Path.Combine(outputFolderPath, chunk.ExtractedPath);
                ExtractChunk(chunk, archiveFile, chunkFilePath);
            }
        }

        public static void ValidateExtractedChunk(ArchiveItem chunk, string chunkFilePath)
        {
            if (!File.Exists(chunkFilePath))
            {
                throw new Exception($"missing file for chunk {chunk.ExtractedPath}. expected file name: {chunkFilePath}");
            }

            BinaryData.ValidateFileHashes(chunkFilePath, chunk.Hashes);
        }

        public static void ValidateExtractedChunks(ArchiveTokenization tokens, string chunksFolderPath)
        {
            foreach (var chunk in tokens.Chunks.Where(c => !c.HasAllZeroRecipe()))
            {
                string chunkFilePath = Path.Combine(chunksFolderPath, chunk.ExtractedPath);
                ValidateExtractedChunk(chunk, chunkFilePath);
            }
        }
    }
}
