/**
 * @file ArchiveTokenization.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Security.Authentication;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Extensions.Options;

namespace ArchiveUtility
{
    public class ArchiveTokenization
    {
        public string Type { get; private set; }
        public string Subtype { get; private set; }
        public ArchiveItemCollection ArchiveItems { get; set; } = new();
        public ArchiveItem[] Chunks => ArchiveItems.Chunks;
        public ArchiveItem[] Payload => ArchiveItems.Payload;
        public ArchiveTokenization(string type, string subtype)
        {
            Type = type;
            Subtype = subtype;
        }
        public class ArchiveItemCollection
        {
            Dictionary<int, ArchiveItem> lookup = new();
            List<ArchiveItem> payload = new();
            List<ArchiveItem> chunks = new();

            public ArchiveItem[] Chunks => chunks.ToArray();
            public ArchiveItem[] Payload => payload.ToArray();
            public bool Contains(int id) => lookup.ContainsKey(id);
            public ArchiveItem Get(int id) => lookup[id];
            protected int NextId
            {
                get
                {
                    return (lookup.Count == 0) ? 0 : lookup.Keys.Max() + 1;
                }
            }

            public void ClearPayload()
            {
                foreach (var item in payload)
                {
                    lookup.Remove(item.Id.GetValueOrDefault());
                }
                payload.Clear();
            }

            public void Add(ArchiveItem item)
            {
                List<ArchiveItem> list;
                switch (item.Type)
                {
                    case ArchiveItemType.Chunk:
                        list = chunks;
                        break;
                    case ArchiveItemType.Payload:
                        list = payload;
                        break;
                    default:
                        throw new FatalException($"Unexpected ArchiveItem type being added to collection for name: {item.Name}, type: {item.Type}");
                }

                if (!item.Id.HasValue || lookup.ContainsKey(item.Id.GetValueOrDefault()))
                {
                    item.Id = NextId;
                }
                list.Add(item);
                lookup.Add(item.Id.GetValueOrDefault(), item);
            }
        }

        public void RemoveRecipesDependentOnArchiveItemId(string id)
        {
            foreach (var chunk in Chunks)
            {
                chunk.Recipes.RemoveAll(recipe => recipe.DependsOnArchiveItemId(id));
            }
        }

        public Dictionary<string, ArchiveItem> BuildPayloadNameToChunkMap()
        {
            Dictionary<string, ArchiveItem> map = new();

            foreach (var chunk in Chunks)
            {
                if (chunk.Recipes.Count == 0)
                {
                    continue;
                }

                if (chunk.Recipes[0].Type != RecipeType.Copy)
                {
                    continue;
                }

                var copyRecipe = chunk.Recipes[0] as CopyRecipe;
                map[copyRecipe.Item.Name] = chunk;
            }
            return map;
        }

        Recipe GetChunkRecipeForExposedPayload(ArchiveItem  payload)
        {
            return new ZstdCompressionRecipe(payload.MakeReference(), 1, 5, 3);
        }

        bool CanValidateChunk(ArchiveItem chunk)
        {
            return chunk.Hashes.ContainsKey(HashAlgorithmType.Sha256);
        }

        bool ExposeTypeRequiresValidation(ExposeCompressedPayloadType exposeType)
        {
            return exposeType == ExposeCompressedPayloadType.ValidatedZstandard;
        }

        bool TryExposeCompressedPayloadFromRecipe(Stream archiveStream, ArchiveItem payload, ArchiveItem chunk, Recipe payloadRecipe, ExposeCompressedPayloadType exposeType)
        {
            var uncompressedFile = Path.GetTempFileName();

            if (ExposeTypeRequiresValidation(exposeType) && !CanValidateChunk(chunk))
            {
                return false;
            }

            using (var uncompressedStream = File.OpenWrite(uncompressedFile))
            {
                try
                {
                    payloadRecipe.CopyTo(archiveStream, uncompressedStream);
                }
                catch (Exception)
                {
                    return false;
                }
            }

            string name = Path.GetFileNameWithoutExtension(payload.Name);

            ulong length = (ulong)new FileInfo(uncompressedFile).Length;
            var sha256Hash = Hash.FromFile(uncompressedFile).ValueString();

            Dictionary<HashAlgorithmType, string> hashes = new() { { HashAlgorithmType.Sha256, sha256Hash } };

            if (exposeType == ExposeCompressedPayloadType.ValidatedZstandard)
            {
                string recompressedFile = uncompressedFile + ".zst";
                ZstdCompressFile.CompressFile(uncompressedFile, recompressedFile);

                var recompressedFileSha256Hash = Hash.FromFile(recompressedFile).ValueString();

                var compressedFileSha256Hash = chunk.Hashes[HashAlgorithmType.Sha256];

                if (!compressedFileSha256Hash.Equals(recompressedFileSha256Hash))
                {
                    return false;
                }
            }

            var newPayload = ArchiveItem.CreatePayload(name, length, hashes);

            newPayload.Recipes.Add(payloadRecipe);
            ArchiveItems.Add(newPayload);

            chunk.Recipes.Clear();
            if (exposeType == ExposeCompressedPayloadType.ValidatedZstandard)
            {
                Recipe newChunkRecipe = GetChunkRecipeForExposedPayload(newPayload);
                if (newChunkRecipe != null)
                {
                    chunk.Recipes.Add(newChunkRecipe);
                }
            }

            return true;
        }

        Recipe GetRecipeForCompressedEntry(Stream archiveStream, ArchiveItem payload, ArchiveItem chunk)
        {
            if (payload.Name.EndsWith(".zst", StringComparison.OrdinalIgnoreCase)
            || payload.Name.EndsWith(".zstd", StringComparison.OrdinalIgnoreCase))
            {
                return new ZstdDecompressionRecipe(chunk.MakeReference());
            }
            else if (payload.Name.EndsWith(".gz", StringComparison.OrdinalIgnoreCase))
            {
                return new GzDecompressionRecipe(chunk.MakeReference());
            }

            return null;
        }

        public bool TryExposePayloadFromChunk(Stream archiveStream, ArchiveItem payload, ArchiveItem chunk, ExposeCompressedPayloadType exposeType)
        {
            Recipe payloadRecipe = GetRecipeForCompressedEntry(archiveStream, payload, chunk);

            if (payloadRecipe == null)
            {
                return false;
            }

            if ((exposeType == ExposeCompressedPayloadType.ValidatedZstandard) && (payloadRecipe.Type != RecipeType.ZstdDecompression))
            {
                return false;
            }

            return TryExposeCompressedPayloadFromRecipe(archiveStream, payload, chunk, payloadRecipe, exposeType);
        }

        public enum ExposeCompressedPayloadType
        {
            None,
            All,
            ValidatedZstandard
        }

        public void ExposeCompressedPayload(Stream archiveStream, ExposeCompressedPayloadType exposeType)
        {
            if (exposeType == ExposeCompressedPayloadType.None)
            {
                return;
            }

            var originalPayload = Payload;

            var map = BuildPayloadNameToChunkMap();
            ArchiveItems.ClearPayload();
            foreach (var payload in originalPayload)
            {
                if (map.ContainsKey(payload.Name))
                {
                    var chunk = map[payload.Name];

                    if (TryExposePayloadFromChunk(archiveStream, payload, chunk, exposeType))
                    {
                        continue;
                    }
                }

                ArchiveItems.Add(payload);
            }
        }

        public void WriteJson(Stream stream, bool writeIndented)
        {
            using (var writer = new StreamWriter(stream, Encoding.UTF8, -1, true))
            {
                var value = ToJson(writeIndented);

                writer.Write(value);
            }
        }

        public string ToJson(bool writeIndented)
        {
            var options = GetStandardJsonSerializerOptions();
            options.WriteIndented = writeIndented;

            return JsonSerializer.Serialize(this, options);
        }

        public string ToJson()
        {
            return ToJson(false);
        }

        public static ArchiveTokenization FromJsonPath(string path)
        {
            using (var stream = File.OpenRead(path))
            {
                return FromJson(stream);
            }
        }

        public static ArchiveTokenization FromJson(Stream stream)
        {
            using (var reader = new StreamReader(stream))
            {
                var json = reader.ReadToEnd();
                return FromJson(json);
            }
        }

        public static ArchiveTokenization FromJson(string json)
        {
            return JsonSerializer.Deserialize<ArchiveTokenization>(json, GetStandardJsonSerializerOptions());
        }

        public static JsonSerializerOptions GetStandardJsonSerializerOptions()
        {
            var options = new JsonSerializerOptions()
            {
                Converters =
                {
                    new ArchiveTokenizationJsonConverter(),
                    new RecipeParameterJsonConverter(),
                    new ArchiveItemJsonConverter(),
                    new RecipeMethodJsonConverter()
                },
                MaxDepth = 128,
            };

            return options;
        }

        public void FillChunkGaps(Stream archiveFile)
        {
            List<ArchiveItem> gapChunks = FindChunkGaps(archiveFile, Chunks.ToList());
            foreach(var gapChunk in gapChunks)
            {
                ArchiveItems.Add(gapChunk);
            }
        }

        public static List<ArchiveItem> FindChunkGaps(Stream archiveFile, List<ArchiveItem> chunks)
        {
            chunks.Sort((c1, c2) =>
            {
                var offsetDiff = c1.Offset.Value - c2.Offset.Value;
                if (offsetDiff != 0)
                {
                    return (int)offsetDiff;
                }
                return (int)(c1.Length - c2.Length);
            });
            List<ArchiveItem> gapChunks = new();

            ulong nextFreePosition = 0;
            archiveFile.Seek(0, SeekOrigin.Begin);

            foreach (ArchiveItem chunk in chunks)
            {
                if (chunk.Offset.Value < nextFreePosition)
                {
                    //ideally, chunks should not overlap. But if they do, we ignore the latter.
                    continue;
                }

                if (chunk.Offset.Value > nextFreePosition)
                {
                    gapChunks.AddRange(ArchiveItemsFromGap(archiveFile, nextFreePosition, chunk.Offset.Value));
                }

                nextFreePosition = chunk.Offset.Value + chunk.Length;
                archiveFile.Seek((long) nextFreePosition, SeekOrigin.Begin);
            }

            if ((ulong) archiveFile.Length > nextFreePosition)
            {
                gapChunks.AddRange(ArchiveItemsFromGap(archiveFile, nextFreePosition, (ulong) archiveFile.Length));
            }

            return gapChunks;
        }

        private static List<ArchiveItem> ArchiveItemsFromGap(Stream archiveFile, ulong gapBegin, ulong gapEnd)
        {
            List<ArchiveItem> gapArchiveItems = new();

            //Align gap chunks on 1024 bytes, to make matching easier
            if (gapBegin % 1024 != 0)
            {
                ulong updatedGapBegin = ((gapBegin >> 10) + 1) << 10;
                if (updatedGapBegin <= gapEnd)
                {
                    gapArchiveItems.Add(ArchiveItemFromGap(archiveFile, gapBegin, updatedGapBegin));
                    gapBegin = updatedGapBegin;
                }
            }

            //the maximum size allowed for gap chunks
            const ulong TEN_MB = 10 * (1 << 20);

            for (ulong archiveItemBegin = gapBegin, archiveItemEnd; archiveItemBegin < gapEnd; archiveItemBegin = archiveItemEnd)
            {
                archiveItemEnd = Math.Min(gapEnd, archiveItemBegin + TEN_MB);

                gapArchiveItems.Add(ArchiveItemFromGap(archiveFile, archiveItemBegin, archiveItemEnd));
            }

            return gapArchiveItems;
        }

        private static ArchiveItem ArchiveItemFromGap(Stream archiveFile, ulong archiveItemBegin, ulong archiveItemEnd)
        {
            ulong archiveItemLength = archiveItemEnd - archiveItemBegin;
            using (BinaryReader reader = new(archiveFile, Encoding.Default, true))
            {
                byte[] buffer = new byte[archiveItemLength];
                if (archiveItemLength != (ulong) reader.Read(buffer, 0, (int)archiveItemLength))
                {
                    throw new FormatException($"Not enough data to read binary chunk of {archiveItemLength} bytes.");
                }

                var chunkId = ArchiveItem.MakeGapChunkId(archiveItemBegin);
                ArchiveItem archiveItem = ArchiveItem.FromByteSpan(chunkId, ArchiveItemType.Chunk, buffer, archiveItemBegin);

                //if buffer is all zeroes, add all-zero recipe
                if (AsciiData.IsAllNul(buffer))
                {
                    archiveItem.Recipes.Add(new AllZeroRecipe(archiveItemLength));
                }

                return archiveItem;
            }
        }
    }

    public class ArchiveTokenizationJsonConverter : JsonConverter<ArchiveTokenization>
    {
        public override ArchiveTokenization Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            reader.CheckStartObject();

            string type = null;
            string subtype = null;
            ArchiveItem[] chunks = null;
            ArchiveItem[] payload = null;

            while (reader.Read())
            {
                if (reader.TokenType == JsonTokenType.EndObject)
                {
                    break;
                }

                if (reader.TokenType != JsonTokenType.PropertyName)
                {
                    throw new JsonException();
                }

                string propertyName = reader.GetString();
                reader.Read();
                switch (propertyName)
                {
                    case "Type":
                        type = reader.GetString();
                        break;
                    case "Subtype":
                        subtype = reader.GetString();
                        break;
                    case "Chunks":
                        chunks = JsonSerializer.Deserialize<ArchiveItem[]>(ref reader, options);
                        break;
                    case "Payload":
                        payload = JsonSerializer.Deserialize<ArchiveItem[]>(ref reader, options);
                        break;
                    default:
                        throw new JsonException();
                }
            }

            ArchiveTokenization tokens = new(type, subtype);
            if (chunks != null)
            {
                foreach (var chunkItem in chunks)
                {
                    if (tokens.ArchiveItems.Contains(chunkItem.Id.Value))
                    {
                        throw new FatalException($"Duplicate Id detected for Chunk: {chunkItem.Id}");
                    }
                    tokens.ArchiveItems.Add(chunkItem);
                }
            }

            if (payload != null)
            {
                foreach (var payloadItem in payload)
                {
                    if (tokens.ArchiveItems.Contains(payloadItem.Id.Value))
                    {
                        throw new FatalException($"Duplicate Id detected for Payload: {payloadItem.Id}");
                    }
                    tokens.ArchiveItems.Add(payloadItem);
                }
            }

            return tokens;
        }

        public override void Write(Utf8JsonWriter writer, ArchiveTokenization tokens, JsonSerializerOptions options)
        {
            writer.WriteStartObject();

            writer.WriteStringProperty("Type", tokens.Type);

            writer.WriteStringProperty("Subtype", tokens.Subtype);

            writer.WritePropertyName("Chunks");
            JsonSerializer.Serialize(writer, tokens.Chunks, options);

            writer.WritePropertyName("Payload");
            JsonSerializer.Serialize(writer, tokens.Payload, options);

            writer.WriteEndObject();
        }
    }
}
