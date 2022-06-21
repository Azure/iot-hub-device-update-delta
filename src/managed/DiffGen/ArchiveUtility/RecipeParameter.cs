/**
 * @file RecipeParameter.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace ArchiveUtility
{
    public enum RecipeParameterType : byte
    {
        ArchiveItem = 0,
        Number = 1,
    }
    public abstract class RecipeParameter
    {
        public abstract RecipeParameterType Type { get; }
        public virtual bool DependsOnArchiveItemId(string id)
        {
            return false;
        }

        public abstract RecipeParameter Export();
        public abstract RecipeParameter DeepCopy();
    }
    public class ArchiveItemRecipeParameter : RecipeParameter
    {
        public ArchiveItem Item { get; }
        public override RecipeParameterType Type => RecipeParameterType.ArchiveItem;
        public ArchiveItemRecipeParameter(ArchiveItem item)
        {
            Item = item.MakeReference();
            Item.Recipes = item.Recipes;
        }
        public override bool DependsOnArchiveItemId(string id)
        {
            return Item.Id.Equals(id);
        }
        public override RecipeParameter Export()
        {
            if (Item.Type == ArchiveItemType.Chunk)
            {
                if (!Item.Offset.HasValue)
                {
                    throw new Exception($"Chunk {Item.Name}({Item.Id}) had no offset.");
                }

                var blob = ArchiveItem.CreateBlob(Item.Name, Item.Length, Item.Hashes);
                blob.Recipes.Add(new CopySourceRecipe(Item.Offset.Value));

                return new ArchiveItemRecipeParameter(blob);
            }

            ArchiveItemRecipeParameter newParam = new(Item);

            for (int i = 0; i < newParam.Item.Recipes.Count; i++)
            {
                newParam.Item.Recipes[i] = newParam.Item.Recipes[i].Export();
            }

            return newParam;
        }

        public override RecipeParameter DeepCopy()
        {
            return new ArchiveItemRecipeParameter(Item.MakeCopy(true));
        }
    }

    public class NumberRecipeParameter : RecipeParameter
    {
        public override RecipeParameterType Type => RecipeParameterType.Number;
        public ulong Number { get; }
        public NumberRecipeParameter(ulong number)
        {
            Number = number;
        }
        public override RecipeParameter Export()
        {
            return DeepCopy();
        }
        public override RecipeParameter DeepCopy()
        {
            return new NumberRecipeParameter(Number);
        }
    }

    public class RecipeParameterJsonConverter : JsonConverter<RecipeParameter>
    {
        public override RecipeParameter Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            reader.CheckStartObject();

            string type = reader.ReadStringProperty("Type");

            RecipeParameter recipeParameter;

            switch (type)
            {
                case "ArchiveItem":
                    ArchiveItem item = reader.ReadObjectProperty<ArchiveItem>("Item", options);
                    recipeParameter = new ArchiveItemRecipeParameter(item);
                    break;
                case "Number":
                    ulong number = reader.ReadUInt64Property("Number");
                    recipeParameter = new NumberRecipeParameter(number);
                    break;
                default:
                    throw new JsonException();
            }

            reader.ReadEndObject();

            return recipeParameter;
        }

        public override void Write(Utf8JsonWriter writer, RecipeParameter value, JsonSerializerOptions options)
        {
            if (value is ArchiveItemRecipeParameter)
            {
                var param = value as ArchiveItemRecipeParameter;
                writer.WriteStartObject();
                writer.WriteStringProperty("Type", "ArchiveItem");
                writer.WritePropertyName("Item");
                JsonSerializer.Serialize(writer, param.Item, options);
                writer.WriteEndObject();
                return;
            }

            if (value is NumberRecipeParameter)
            {
                var param = value as NumberRecipeParameter;
                writer.WriteStartObject();
                writer.WriteStringProperty("Type", "Number");
                writer.WriteNumberProperty("Number", param.Number);
                writer.WriteEndObject();
                return;
            }

            throw new Exception($"Unsupported type with RecipeParameterJsonConverter.Write(). Type: {typeof(RecipeParameter).Name}");
        }
    }
}
