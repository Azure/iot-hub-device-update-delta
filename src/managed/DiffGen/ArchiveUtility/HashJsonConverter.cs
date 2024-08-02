/**
 * @file HashJsonConverter.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Security.Authentication;
    using System.Text.Json;
    using System.Text.Json.Serialization;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class HashJsonConverter : JsonConverter<Hash>
    {
        public override Hash Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            reader.CheckStartObject();

            HashAlgorithmType? type = null;
            byte[] value = null;

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
                    case "type":
                        type = (HashAlgorithmType)reader.GetUInt64();
                        break;
                    case "value":
                        var hashString = reader.GetString();
                        value = HexUtility.HexStringToByteArray(hashString);
                        break;
                    default:
                        throw new JsonException();
                }
            }

            if (type == null || value == null)
            {
                throw new JsonException();
            }

            return new(type.Value, value);
        }

        public override void Write(Utf8JsonWriter writer, Hash value, JsonSerializerOptions options)
        {
            writer.WriteStartObject();
            writer.WriteNumberProperty("type", (UInt64)value.Type);
            var hexString = HexUtility.ByteArrayToHexString(value.Value);
            writer.WriteStringProperty("value", hexString);
            writer.WriteEndObject();
        }
    }
}
