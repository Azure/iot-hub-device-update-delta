/**
 * @file PayloadExtractor.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.IO;

    public class PayloadExtractor
    {
        public static void ExtractPayload(ArchiveTokenization tokens, Stream archiveStream, string outputFolderPath)
        {
            if (!Directory.Exists(outputFolderPath))
            {
                Directory.CreateDirectory(outputFolderPath);
            }

            foreach (var payload in tokens.Payload)
            {
                if (payload.Recipes.Count == 0)
                {
                    throw new Exception($"No recipe found for payload: {payload.Id}");
                }

                if (payload.Name.Equals(".") || payload.Name.EndsWith("\\."))
                {
                    if (payload.Length != 0)
                    {
                        throw new Exception($"Found non-empty directory when trying to expand payload. Id: {payload.Id}, Name: {payload.Name}");
                    }
                    continue;
                }

                if (payload.Length == 0)
                {
                    continue;
                }

                string outputFilePath = Path.Combine(outputFolderPath, payload.ExtractedPath);

                try
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(outputFilePath));
                    using var writeStream = File.Create(outputFilePath);
                    payload.CopyTo(archiveStream, writeStream);
                }
                catch (Exception e)
                {
                    throw new Exception($"Couldn't create payload for payload. Id: {payload.Id}, Name: {payload.Name}", e);
                }
            }
        }

        public static void ValidateExtractedPayload(ArchiveTokenization tokens, string payloadsFolderPath)
        {
            foreach (var payload in tokens.Payload)
            {
                if (payload.Name.Equals(".") || payload.Name.EndsWith("\\."))
                {
                    continue;
                }

                if (payload.Length == 0)
                {
                    continue;
                }

                string payloadFilePath = Path.Combine(payloadsFolderPath, payload.ExtractedPath);
                if (!File.Exists(payloadFilePath))
                {
                    throw new Exception($"missing file for payload {payload.Name}. expected file name: {payloadFilePath}");
                }

                BinaryData.ValidateFileHashes(payloadFilePath, payload.Hashes);
            }
        }
    }
}
