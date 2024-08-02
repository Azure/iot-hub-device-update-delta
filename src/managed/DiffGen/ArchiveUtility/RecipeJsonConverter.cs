/**
 * @file RecipeJsonConverter.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Text.Json;
    using System.Text.Json.Serialization;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class RecipeJsonConverter : JsonConverter<Recipe>
    {
        public override Recipe Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            reader.CheckStartObject();

            string name = null;
            ItemDefinition result = null;
            List<UInt64> numbers = new();
            List<ItemDefinition> items = new();

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
                    case "Name":
                        name = reader.GetString();
                        break;
                    case "Result":
                        result = JsonSerializer.Deserialize<ItemDefinition>(ref reader, options);
                        break;
                    case "NumberIngredients":
                        numbers = JsonSerializer.Deserialize<List<UInt64>>(ref reader, options);
                        break;
                    case "ItemIngredients":
                        items = JsonSerializer.Deserialize<List<ItemDefinition>>(ref reader, options);
                        break;
                    default:
                        throw new JsonException();
                }
            }

            if (name == null || result == null)
            {
                throw new JsonException();
            }

            return new(name, result, numbers, items);
        }

        public override void Write(Utf8JsonWriter writer, Recipe value, JsonSerializerOptions options)
        {
            writer.WriteStartObject();
            writer.WriteStringProperty("Name", value.Name);
            writer.WritePropertyName("Result");
            JsonSerializer.Serialize(writer, value.Result, options);
            writer.WritePropertyName("NumberIngredients");
            JsonSerializer.Serialize(writer, value.NumberIngredients, options);
            writer.WritePropertyName("ItemIngredients");
            JsonSerializer.Serialize(writer, value.ItemIngredients, options);
            writer.WriteEndObject();
        }
    }
}
