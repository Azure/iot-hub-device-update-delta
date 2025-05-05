/**
 * @file ArchiveTokenizationJsonConverter.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Linq;
    using System.Text.Json;
    using System.Text.Json.Serialization;

    using SerializedPayloadList = System.Collections.Generic.List<System.Collections.Generic.KeyValuePair<ArchiveUtility.Payload, System.Collections.Generic.HashSet<ArchiveUtility.ItemDefinition>>>;
    using SerializedRecipeList = System.Collections.Generic.List<System.Collections.Generic.KeyValuePair<ArchiveUtility.ItemDefinition, ArchiveUtility.Recipe>>;
    using SerializedRecipeSetList = System.Collections.Generic.List<System.Collections.Generic.KeyValuePair<ArchiveUtility.ItemDefinition, System.Collections.Generic.HashSet<ArchiveUtility.Recipe>>>;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class ArchiveTokenizationJsonConverter : JsonConverter<ArchiveTokenization>
    {
        public override ArchiveTokenization Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            reader.CheckStartObject();

            string type = null;
            string subtype = null;
            ItemDefinition archiveItem = null;
            ItemDefinition sourceItem = null;
            SerializedPayloadList payload = null;
            List<Recipe> recipes = null;
            List<Recipe> forwardRecipes = null;
            List<Recipe> reverseRecipes = null;

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
                    case "Type":
                        type = reader.GetString();
                        break;
                    case "Subtype":
                        subtype = reader.GetString();
                        break;
                    case "ArchiveItem":
                        archiveItem = JsonSerializer.Deserialize<ItemDefinition>(ref reader, options);
                        break;
                    case "SourceItem":
                        sourceItem = JsonSerializer.Deserialize<ItemDefinition>(ref reader, options);
                        break;
                    case "Payload":
                        payload = JsonSerializer.Deserialize<SerializedPayloadList>(ref reader, options);
                        break;
                    case "Recipes":
                        recipes = JsonSerializer.Deserialize<List<Recipe>>(ref reader, options);
                        break;
                    case "ForwardRecipes":
                        forwardRecipes = JsonSerializer.Deserialize<List<Recipe>>(ref reader, options);
                        break;
                    case "ReverseRecipes":
                        reverseRecipes = JsonSerializer.Deserialize<List<Recipe>>(ref reader, options);
                        break;
                    default:
                        throw new JsonException();
                }
            }

            if (type == null || subtype == null || archiveItem == null)
            {
                throw new JsonException();
            }

            ArchiveTokenization tokens = new(type, subtype);

            tokens.ArchiveItem = archiveItem;

            if (sourceItem != null)
            {
                tokens.SourceItem = sourceItem;
            }

            if (payload != null)
            {
                tokens.SetPayload(payload);
            }

            if (recipes != null)
            {
                tokens.SetRecipes(recipes);
            }

            if (forwardRecipes != null)
            {
                tokens.ForwardRecipes = forwardRecipes.Select(x => new KeyValuePair<ItemDefinition, Recipe>(x.Result, x)).ToDictionary();
            }

            if (reverseRecipes != null)
            {
                tokens.ReverseRecipes = reverseRecipes.Select(x => new KeyValuePair<ItemDefinition, Recipe>(x.Result, x)).ToDictionary();
            }

            return tokens;
        }

        public override void Write(Utf8JsonWriter writer, ArchiveTokenization value, JsonSerializerOptions options)
        {
            writer.WriteStartObject();
            writer.WriteStringProperty("Type", value.Type);
            writer.WriteStringProperty("Subtype", value.Subtype);
            writer.WritePropertyName("ArchiveItem");
            JsonSerializer.Serialize(writer, value.ArchiveItem, options);
            if (value.SourceItem != null)
            {
                writer.WritePropertyName("SourceItem");
                JsonSerializer.Serialize(writer, value.SourceItem, options);
            }

            writer.WritePropertyName("Payload");
            JsonSerializer.Serialize(writer, value.Payload, options);

            writer.WritePropertyName("Recipes");
            JsonSerializer.Serialize(writer, value.Recipes, options);

            writer.WritePropertyName("ForwardRecipes");
            JsonSerializer.Serialize(writer, value.ForwardRecipes.Values.ToList(), options);

            writer.WritePropertyName("ReverseRecipes");
            JsonSerializer.Serialize(writer, value.ReverseRecipes.Values.ToList(), options);

            writer.WriteEndObject();
        }
    }
}
