/**
 * @file ArchiveItem.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.Security.Authentication;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Linq;

namespace ArchiveUtility
{
    public enum ArchiveItemType : sbyte
    {
        Unknown = -1,
        Blob = 0,
        Chunk = 1,
        Payload = 2,
    }

    public record ArchiveItem(
        string Name,
        ArchiveItemType Type,
        UInt64? Offset,
        UInt64 Length,
        string DevicePath,
        Dictionary<HashAlgorithmType, string> Hashes)
    {
        public static ArchiveItem CreateChunk(string name, ulong offset, ulong length, Dictionary<HashAlgorithmType, string> hashes)
        {
            return new ArchiveItem(name, ArchiveItemType.Chunk, offset, length, null, hashes);
        }
        public static ArchiveItem CreatePayload(string name, ulong length, Dictionary<HashAlgorithmType, string> hashes)
        {
            return new ArchiveItem(name, ArchiveItemType.Payload, null, length, null, hashes);
        }
        public static ArchiveItem CreateBlob(string name, ulong length, Dictionary<HashAlgorithmType, string> hashes)
        {
            return new ArchiveItem(name, ArchiveItemType.Blob, null, length, null, hashes);
        }

        private ArchiveItem reference = null;
        public int? id;
        public int? Id
        {
            get
            {
                if (reference == null)
                {
                    return id;
                }
                return reference.Id;
            }
            set
            {
                if (reference == null)
                {
                    id = value;
                    return;
                }
                throw new Exception("Attempting to set id on ArchiveItem reference.");
            }
        }

        public string ExtractedPath
        {
            get
            {
                if (!Id.HasValue)
                {
                    throw new Exception("Can't determine ExtractedPath for item with no id");
                }

                return Id.Value.ToString();
            }
        }

        public List<Recipe> Recipes { get; set; } = new();
        public Hash ArchiveHash { get; set; } = null;

        public static ArchiveItem FromByteSpan(string name, ArchiveItemType type, ReadOnlySpan<byte> data, UInt64? offset = null)
        {
            return new ArchiveItem(name, type, offset, (UInt64)data.Length, null, BinaryData.CalculateHashes(data));
        }

        public static ArchiveItem FromBinaryReader(string name, ArchiveItemType type, BinaryReader reader, UInt64 length, UInt64? offset = null)
        {
            return new ArchiveItem(name, type, offset, length, null, BinaryData.CalculateHashes(reader, length));
        }
        public static ArchiveItem FromStream(string name, ArchiveItemType type, Stream stream)
        {
            // Some streams don't support the Length property and would throw an 
            // exception, so instead calculate the length while hashing.
            var hashes = BinaryData.GetHashesAndLengthOfStream(stream, out UInt64 length);
            return new ArchiveItem(name, type, null, length, null, hashes);
        }

        public ArchiveItem MakeReference()
        {
            return new ArchiveItem(Name, Type, Offset, Length, DevicePath, Hashes) { reference = this, ArchiveHash = this.ArchiveHash };
        }

        public ArchiveItem MakeCopy(bool copyRecipes)
        {
            var copy = new ArchiveItem(Name, Type, Offset, Length, DevicePath, Hashes) { ArchiveHash = this.ArchiveHash };
            if (copyRecipes)
            {
                copy.Recipes = Recipes;
            }

            return copy;
        }

        public ArchiveItem ReplaceItem(ArchiveItem itemOld, ArchiveItem itemNew)
        {
            if (Equals(itemOld) || Hashes.Equals(itemOld.Hashes))
            {
                return itemNew.MakeCopy(true);
            }

            var copy = MakeCopy(true);

            for (int i = 0; i < copy.Recipes.Count; i++)
            {
                copy.Recipes[i] = copy.Recipes[i].ReplaceItem(itemOld, itemNew);
            }

            return copy;
        }

        public void CopyTo(Stream archiveStream, Stream writeStream)
        {
            if (Type == ArchiveItemType.Chunk)
            {
                using (var subStream = new SubStream(archiveStream, (long)Offset.Value, (long)Length))
                {
                    subStream.CopyTo(writeStream);
                }
                return;
            }

            Recipes[0].CopyTo(archiveStream, writeStream);
        }

        public bool HasAllZeroRecipe()
        {
            return Recipes.Any(r => r is AllZeroRecipe);
        }

        public static string MakeChunkName(string payloadName)
        {
            return $"Chunk.Payload={payloadName}";
        }

        public static string MakePaddingChunkName(UInt64 offset)
        {
            return $"Chunk.Padding@{offset}";
        }
        public static string MakeHeaderChunkName(string payloadName)
        {
            return $"Chunk.Header.Payload={payloadName}";
        }

        public static string MakeAllZeroChunkName(UInt64 offset)
        {
            return $"Chunk.Zeroes@{offset}";
        }

        public static string MakeGapChunkId(UInt64 offset)
        {
            return $"Chunk.Gap@{offset}";
        }
    }

    public class ArchiveItemJsonConverter : JsonConverter<ArchiveItem>
    {
        public override ArchiveItem Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            reader.CheckStartObject();

            int? id = null;
            string name = null;
            ArchiveItemType type = ArchiveItemType.Unknown;
            Hash archiveHash = null;
            UInt64? offset = null;
            UInt64? length = null;
            string devicePath = null;
            Dictionary<HashAlgorithmType, string> hashes = null;
            List<Recipe> recipes = new();

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
                    case "Id":
                        id = reader.GetInt32();
                        break;
                    case "Name":
                        name = reader.GetString();
                        break;
                    case "Type":
                        if (!Enum.TryParse(reader.GetString(), ignoreCase: false, out type))
                        {
                            throw new JsonException();
                        }
                        break;
                    case "Offset":
                        offset = reader.GetUInt64();
                        break;
                    case "Length":
                        length = reader.GetUInt64();
                        break;
                    case "DevicePath":
                        devicePath = reader.GetString();
                        break;
                    case "Hashes":
                        hashes = JsonSerializer.Deserialize<Dictionary<HashAlgorithmType, string>>(ref reader, options);
                        break;
                    case "Recipes":
                        recipes = JsonSerializer.Deserialize<List<Recipe>>(ref reader, options);
                        break;
                    case "ArchiveHash":
                        archiveHash = JsonSerializer.Deserialize<Hash>(ref reader, options);
                        break;
                    default:
                        throw new JsonException();
                }
            }

            //validate the mandatory fields
            List<string> missingMandatoryFields = new();
            if (id == null)
            {
                missingMandatoryFields.Add("Id");
            }
            if (name == null)
            {
                missingMandatoryFields.Add("Name");
            }
            if (type == ArchiveItemType.Unknown)
            {
                missingMandatoryFields.Add("Type");
            }
            if (length == null)
            {
                missingMandatoryFields.Add("Length");
            }
            if (hashes == null)
            {
                missingMandatoryFields.Add("Hashes");
            }
            if (missingMandatoryFields.Count > 0)
            {
                throw new Exception("ArchiveItem missing mandatory fields: " + string.Join(", ", missingMandatoryFields));
            }

            return new ArchiveItem(name, type, offset, length.Value, devicePath, hashes) { Id = id, Recipes = recipes, ArchiveHash = archiveHash };
        }

        public override void Write(Utf8JsonWriter writer, ArchiveItem value, JsonSerializerOptions options)
        {
            writer.WriteStartObject();

            writer.WriteNumberProperty("Id", value.Id.GetValueOrDefault());

            if (!string.IsNullOrEmpty(value.Name))
            {
                writer.WriteStringProperty("Name", value.Name);
            }
            writer.WriteStringProperty("Type", value.Type.ToString());
            if (value.Offset.HasValue)
            {
                writer.WriteNumberProperty("Offset", value.Offset.Value);
            }
            writer.WriteNumberProperty("Length", value.Length);
            if (!string.IsNullOrEmpty(value.DevicePath))
            {
                writer.WriteStringProperty("DevicePath", value.DevicePath);
            }
            if (value.ArchiveHash != null)
            {
                writer.WritePropertyName("ArchiveHash");
                JsonSerializer.Serialize(writer, value.ArchiveHash, options);
            }
            if (value.Hashes.Count != 0)
            {
                writer.WritePropertyName("Hashes");
                JsonSerializer.Serialize(writer, value.Hashes, options);
            }
            if (value.Recipes.Count != 0)
            {
                writer.WritePropertyName("Recipes");
                JsonSerializer.Serialize(writer, value.Recipes, options);
            }
            writer.WriteEndObject();
        }
    }
}
