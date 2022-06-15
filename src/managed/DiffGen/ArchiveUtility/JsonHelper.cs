/**
 * @file JsonHelper.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace ArchiveUtility
{
    public static class JsonHelper
    {
        public static void CheckStartObject(this ref Utf8JsonReader reader)
        {
            if (reader.TokenType != JsonTokenType.StartObject)
            {
                throw new JsonException("the start of an object was expected");
            }
        }

        public static void ReadPropertyKey(this ref Utf8JsonReader reader, string key)
        {
            reader.Read();
            if (reader.TokenType != JsonTokenType.PropertyName ||
                reader.GetString() != key)
            {
                throw new JsonException($"the next property in the JSON is expected to be called \"{key}\"");
            }
        }

        public static string ReadStringProperty(this ref Utf8JsonReader reader, string key)
        {
            ReadPropertyKey(ref reader, key);

            reader.Read();
            if (reader.TokenType != JsonTokenType.String)
            {
                throw new JsonException("the next property in the JSON is expected to have string type");
            }

            return reader.GetString();
        }

        public static UInt64 ReadUInt64Property(this ref Utf8JsonReader reader, string key)
        {
            ReadPropertyKey(ref reader, key);

            reader.Read();
            if (reader.TokenType != JsonTokenType.Number)
            {
                throw new JsonException("the next property in the JSON is expected to have number type");
            }

            return reader.GetUInt64();
        }

        public static TValue ReadObjectProperty<TValue>(this ref Utf8JsonReader reader, string key, JsonSerializerOptions options)
        {
            ReadPropertyKey(ref reader, key);

            reader.Read();
            return JsonSerializer.Deserialize<TValue>(ref reader, options);
        }

        public static void ReadEndObject(this ref Utf8JsonReader reader)
        {
            reader.Read();
            if (reader.TokenType != JsonTokenType.EndObject)
            {
                throw new JsonException("end of object expected");
            }
        }

        public static void WriteStringProperty(this Utf8JsonWriter writer, string key, string value)
        {
            writer.WritePropertyName(key);
            writer.WriteStringValue(value);
        }

        public static void WriteNumberProperty(this Utf8JsonWriter writer, string key, ulong value)
        {
            writer.WritePropertyName(key);
            writer.WriteNumberValue(value);
        }

        public static void WriteNumberProperty(this Utf8JsonWriter writer, string key, int value)
        {
            writer.WritePropertyName(key);
            writer.WriteNumberValue(value);
        }

        public static void Deserialize<T>(string path, out T value)
        {
            using (var reader = File.OpenText(path))
            {
                var json = reader.ReadToEnd();
                value = JsonSerializer.Deserialize<T>(json);
            }
        }

        public static void Serialize<T>(T value, string path)
        {
            using (var writer = File.CreateText(path))
            {
                var json = JsonSerializer.Serialize(value, new JsonSerializerOptions() { WriteIndented = true });
                writer.Write(json);
            }
        }
    }
}
