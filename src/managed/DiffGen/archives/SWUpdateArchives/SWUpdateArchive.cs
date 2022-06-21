/**
 * @file SWUpdateArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using ArchiveUtility;
using System.IO;
using System.IO.Compression;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text;
using System.Security.Authentication;

namespace SWUpdateArchives
{
    public class CompressionDetails
    {
        [JsonPropertyName("type")]
        public string Type { get; set; }
        [JsonPropertyName("level")]
        public UInt64 Level { get; set; }
        [JsonPropertyName("major-version")]
        public UInt64 MajorVersion { get; set; }
        [JsonPropertyName("minor-version")]
        public UInt64 MinorVersion { get; set; }
        [JsonPropertyName("original-file-name")]
        public string OriginalFileName { get; set; }
        [JsonPropertyName("compressed-file-name")]
        public string CompressedFileName { get; set; }
        [JsonPropertyName("original-file-sha256hash")]
        public string OriginalFileSha256Hash { get; set; }
        [JsonPropertyName("compressed-file-sha256hash")]
        public string CompressedFileSha256Hash { get; set; }
        [JsonPropertyName("original-file-size")]
        public UInt64 OriginalFileSize{ get; set; }
        [JsonPropertyName("compressed-file-size")]
        public UInt64 CompressedFileSize { get; set; }
    }

    public class SWUpdateArchive : IArchive
    {
        private ArchiveLoaderContext Context;
        public SWUpdateArchive(ArchiveLoaderContext context)
        {
            Context = context;
        }
        public string ArchiveType { get { return "SWUpdate"; } }

        public string ArchiveSubtype { get { return "base"; } }

        private const string SWUpdateDescriptionFile = "sw-description";
        private const string SWUpdateDescriptionSigFile = "sw-description.sig";
        private const string CompressionDetailsFile = "compression-details.json";

        public bool IsMatchingFormat()
        {
            if (!ArchiveLoader.TryLoadArchive(Context, out ArchiveTokenization tokens, "cpio", "tar"))
            {
                return false;
            }

            return tokens.Payload.Length > 1 && tokens.Payload[0].Name.Equals(SWUpdateDescriptionFile);
        }

        const string ZstdCompressionType = "zstd";

        Dictionary<string, CompressionDetails> GetCompressionDetails(ArchiveTokenization tokens)
        {
            Dictionary<string, CompressionDetails> details = new();

            foreach (var payload in tokens.Payload)
            {
                if (payload.Name.Equals(CompressionDetailsFile))
                {
                    MemoryStream memoryStream = new();
                    payload.Recipes[0].CopyTo(Context.Stream, memoryStream);
                    string json = Encoding.ASCII.GetString(memoryStream.ToArray());

                    return JsonSerializer.Deserialize<Dictionary<string, CompressionDetails>>(json);
                }
            }

            return new Dictionary<string, CompressionDetails>();
        }

        public ArchiveTokenization Tokenize()
        {
            if (Context.TokenCache.ContainsKey(GetType()))
            {
                return Context.TokenCache[GetType()];
            }

            if (!ArchiveLoader.TryLoadArchive(Context, out ArchiveTokenization parentTokens, "cpio", "tar"))
            {
                throw new FormatException("SWUpdate files are based on either 'cpio' or 'tar' format archives.");
            }

            // We may later want to have the subtype contain SWUpdate version information, but for now
            // the parent archive type is relavant to us, but the SWUpdate version is not
            ArchiveTokenization tokens = new("SWUpdate", parentTokens.Type + "." + parentTokens.Subtype);

            foreach (var chunk in parentTokens.Chunks)
            {
                tokens.ArchiveItems.Add(chunk.MakeCopy(true));
            }

            var compressedPayloadDetails = GetCompressionDetails(parentTokens);
            var payloadNameToChunkMap = tokens.BuildPayloadNameToChunkMap();

            foreach (var payload in parentTokens.Payload)
            {
                if (!payloadNameToChunkMap.ContainsKey(payload.Name))
                {
                    throw new Exception($"Couldn't find chunk for payload: {payload.Name}");
                }

                var chunk = payloadNameToChunkMap[payload.Name];

                var newPayload = ArchiveItem.CreatePayload(payload.Name, payload.Length, payload.Hashes);
                newPayload.Recipes.Add(new CopyRecipe(chunk.MakeReference()));
                tokens.ArchiveItems.Add(newPayload);

                chunk.Recipes.Clear();
                chunk.Recipes.Add(new CopyRecipe(newPayload.MakeReference()));
            }

            Context.TokenCache[GetType()] = tokens;
            return tokens;
        }
    }
}
