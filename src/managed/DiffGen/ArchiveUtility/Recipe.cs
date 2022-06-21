/**
 * @file Recipe.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.Json.Serialization;
using System.Text.Json;

namespace ArchiveUtility
{
    public enum RecipeType : byte
    {
        Copy = 0,
        Region,
        Concatenation,
        ApplyBsDiff,
        ApplyNestedDiff,
        Remainder,
        InlineAsset,
        CopySource,
        ApplyZstdDelta,
        InlineAssetCopy,
        ZstdCompression,
        ZstdDecompression,
        AllZero,
        GzDecompression,
    }

    public abstract class Recipe
    {
        public abstract string Name { get; }
        public abstract RecipeType Type { get; }
        protected abstract List<string> ParameterNames { get; }
        private Dictionary<string, RecipeParameter> ParameterDictionary { get; set; } = new();
        public Recipe()
        {
        }
        public Recipe(params RecipeParameter[] recipeParameters)
        {
            ParameterDictionary = InitializeParameters(recipeParameters);
        }
        public Recipe(params ArchiveItem[] archiveItemParameters)
        {
            var recipeParameters = archiveItemParameters.Select(r => new ArchiveItemRecipeParameter(r)).ToArray();
            ParameterDictionary = InitializeParameters(recipeParameters);
        }

        public virtual void SetParameter(string name, RecipeParameter value)
        {
            if (!ParameterNames.Contains(name))
            {
                throw new Exception($"Trying to add unsupported parameter to recipe. RecipeType: {this.Type}, Name: {name}");
            }

            ParameterDictionary[name] = value;
        }
        public RecipeParameter GetParameter(string name)
        {
            return ParameterDictionary[name];
        }

        public IEnumerable<Tuple<string, RecipeParameter>> GetParameters()
        {
            if (ParameterNames.Count != ParameterDictionary.Count)
            {
                throw new Exception($"GetParameters(): Invalid Recipe Parameters: ParameterNames.Count ({ParameterNames.Count}) must match ParameterDictionary.Count ({ParameterDictionary.Count})");
            }
            return ParameterNames.Select(n => new Tuple<string, RecipeParameter>(n, ParameterDictionary[n]));
        }
        public virtual void CopyTo(Stream archiveStream, Stream stream)
        {
            throw new NotImplementedException();
        }
        public virtual Recipe ReplaceItem(ArchiveItem itemOld, ArchiveItem itemNew)
        {
            var copy = DeepCopy();

            var parameters = GetParameters();

            foreach (var parameter in parameters)
            {
                var paramName = parameter.Item1;
                var paramValue = parameter.Item2;

                if (paramValue.Type == RecipeParameterType.ArchiveItem)
                {
                    var itemParam = paramValue as ArchiveItemRecipeParameter;
                    var item = itemParam.Item;
                    var paramReplaced = new ArchiveItemRecipeParameter(item.ReplaceItem(itemOld, itemNew));

                    copy.SetParameter(paramName, paramReplaced);
                }
            }

            return copy;
        }
        public abstract Recipe DeepCopy();
        protected ulong GetNumberParameter(string name)
        {
            if (!ParameterDictionary.TryGetValue(name, out RecipeParameter param))
            {
                throw new Exception($"{name} wasn't present.");
            }
            var numberParam = (NumberRecipeParameter)param;
            return numberParam.Number;
        }
        protected ArchiveItem GetArchiveItemParameter(string name)
        {
            if (!ParameterDictionary.TryGetValue(name, out RecipeParameter param))
            {
                throw new Exception($"{name} wasn't present.");
            }
            var archiveItemParameter = (ArchiveItemRecipeParameter)param;
            return archiveItemParameter.Item;
        }
        protected virtual Dictionary<string, RecipeParameter> InitializeParameters(RecipeParameter[] recipeParameters)
        {
            var parameterDictionary = new Dictionary<string, RecipeParameter>();

            if (recipeParameters.Length != 0)
            {
                if (recipeParameters.Length != ParameterNames.Count)
                {
                    throw new Exception($"Expecting {ParameterNames.Count} parameter(s), but there are {recipeParameters.Length} parameter(s).");
                }

                for (int i = 0; i < recipeParameters.Length; i++)
                {
                    parameterDictionary[ParameterNames[i]] = recipeParameters[i];
                }
            }

            return parameterDictionary;
        }
        public virtual void SetParameters(Dictionary<string, RecipeParameter> parameterDictionary)
        {
            if (ParameterDictionary.Count != 0)
            {
                throw new Exception("Cannot set ParametersDictionary after it has been initialized.");
            }

            ParameterDictionary = parameterDictionary;
        }
        public static Recipe ReplaceItem(Recipe recipe, ArchiveItem itemOld, ArchiveItem itemNew)
        {
            if (recipe.Type == RecipeType.Copy)
            {
                var copyRecipe = recipe as CopyRecipe;
                if (copyRecipe.Item == itemOld)
                {
                    return itemNew.Recipes[0];
                }

                return recipe;
            }

            return recipe.ReplaceItem(itemOld, itemNew);
        }
        public static Recipe DeepCopy<T>(Recipe original) where T : Recipe, new()
        {
            var copy = new T();

            var parameters = original.GetParameters();

            foreach (var parameter in parameters)
            {
                var paramName = parameter.Item1;
                var paramValue = parameter.Item2;
                RecipeParameter paramCopy = paramValue.DeepCopy();

                copy.SetParameter(paramName, paramCopy);
            }

            return copy;
        }

        public bool DependsOnArchiveItemId(string id)
        {
            return ParameterDictionary.Values.Any(param => param.DependsOnArchiveItemId(id));
        }
        public IList<ArchiveItem> GetDependencies()
        {
            List<ArchiveItem> dependencies = new();

            foreach (var param in ParameterDictionary.Values)
            {
                switch (param.Type)
                {
                    case RecipeParameterType.ArchiveItem:
                        var item = ((ArchiveItemRecipeParameter)param).Item;

                        if (item.Recipes.Count > 0)
                        {
                            var recipe = item.Recipes[0];
                            dependencies.AddRange(recipe.GetDependencies());
                        }

                        dependencies.Add(item);

                        break;
                    default:
                        break;
                }
            }

            return dependencies;
        }
        public virtual bool IsCompressedDependency(ArchiveItem dependency)
        {
            foreach (var param in ParameterDictionary.Values)
            {
                switch (param.Type)
                {
                    case RecipeParameterType.ArchiveItem:
                        var item = ((ArchiveItemRecipeParameter)param).Item;

                        if (item.Recipes.Count > 0)
                        {
                            return item.Recipes[0].IsCompressedDependency(dependency);
                        }
                        break;
                    default:
                        break;
                }
            }

            return false;
        }

        public Recipe Export()
        {
            if (Type == RecipeType.Copy)
            {
                var copyRecipe = this as CopyRecipe;
                var item = copyRecipe.Item;
                if (item.Type == ArchiveItemType.Chunk)
                {
                    if (!item.Offset.HasValue)
                    {
                        throw new Exception($"Chunk {item.Name}({item.Id}) had no offset.");
                    }
                    return new CopySourceRecipe(item.Offset.Value);
                }
            }

            var newRecipe = DeepCopy();
            foreach (var paramName in newRecipe.ParameterDictionary.Keys)
            {
                var paramValue = newRecipe.ParameterDictionary[paramName];
                newRecipe.ParameterDictionary[paramName] = paramValue.Export();
            }
            return newRecipe;
        }

        public static Recipe Create(string recipeName)
        {
            var assembly = Assembly.GetAssembly(typeof(Recipe));

            var assemblyName = assembly.GetName().Name;

            string recipeTypeName = $"{assemblyName}.{recipeName}Recipe";

            Type recipeType = assembly.GetType(recipeTypeName);
            if (recipeType == null)
            {
                throw new Exception($"Could not find recipe for name: {recipeName}");
            }

            var ctor = recipeType.GetConstructor(new Type[0]);

            var recipe = (Recipe) ctor.Invoke(new object[0]);

            return recipe;
        }
    }

    public class RecipeMethodJsonConverter : JsonConverter<Recipe>
    {
        public override Recipe Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            reader.CheckStartObject();

            string name = reader.ReadStringProperty("Name");

            Recipe recipe = Recipe.Create(name);

            var parameterDictionary = reader.ReadObjectProperty<Dictionary<string, RecipeParameter>>("Parameters", options);

            recipe.SetParameters(parameterDictionary);

            reader.ReadEndObject();

            return recipe;
        }

        public override void Write(Utf8JsonWriter writer, Recipe value, JsonSerializerOptions options)
        {
            writer.WriteStartObject();

            writer.WriteStringProperty("Name", value.Name);

            writer.WritePropertyName("Parameters");

            Dictionary<string, RecipeParameter> paramDictionary = new();
            var parameters = value.GetParameters();
            foreach (var parameter in parameters)
            {
                paramDictionary[parameter.Item1] = parameter.Item2;
            }

            JsonSerializer.Serialize(writer, paramDictionary, options);

            writer.WriteEndObject();
        }
    }
}
