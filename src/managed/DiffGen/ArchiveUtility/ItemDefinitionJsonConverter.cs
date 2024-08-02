/**
 * @file ItemDefinitionJsonConverter.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Security.Authentication;
    using System.Text.Json;
    using System.Text.Json.Serialization;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class ItemDefinitionJsonConverter : JsonConverter<ItemDefinition>
    {
        public override ItemDefinition Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            reader.CheckStartObject();

            UInt64? length = null;
            Dictionary<HashAlgorithmType, Hash> hashes = new();
            List<string> names = new();

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

                string property = reader.GetString();

                if (property == null)
                {
                    throw new JsonException();
                }

                reader.Read();

                switch (property)
                {
                    case "Length":
                        length = reader.GetUInt64();
                        break;
                    case "Hashes":
                        hashes = JsonSerializer.Deserialize<Dictionary<HashAlgorithmType, Hash>>(ref reader, options);
                        break;
                    case "Names":
                        names = JsonSerializer.Deserialize<List<string>>(ref reader, options);
                        break;
                    default:
                        throw new JsonException();
                }
            }

            if (length == null)
            {
                throw new JsonException();
            }

            return new(length.Value, hashes, names);
        }

        public override void Write(Utf8JsonWriter writer, ItemDefinition value, JsonSerializerOptions options)
        {
            writer.WriteStartObject();
            writer.WriteNumberProperty("Length", value.Length);
            writer.WritePropertyName("Hashes");
            JsonSerializer.Serialize(writer, value.Hashes, options);
            writer.WritePropertyName("Names");
            JsonSerializer.Serialize(writer, value.Names, options);
            writer.WriteEndObject();
        }
    }
}
