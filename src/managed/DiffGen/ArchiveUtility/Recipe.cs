/**
 * @file Recipe.cs
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

    public enum RecipeType : byte
    {
        AllZeros,
        Chain,
        Slice,
        BsPatchDecompression,
        ZlibCompression,
        ZlibDecompression,
        ZstdCompression,
        ZstdDecompression,
    }

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class Recipe
    {
        public string Name { get; init; }

        public ItemDefinition Result { get; init; }

        public List<UInt64> NumberIngredients { get; init; }

        public List<ItemDefinition> ItemIngredients { get; init; }

        private int? HashCodeCache = null;

        public static string RecipeTypeToString(RecipeType type)
        {
            switch (type)
            {
            case RecipeType.AllZeros: return "all_zeros";
            case RecipeType.Chain: return "chain";
            case RecipeType.Slice: return "slice";
            case RecipeType.BsPatchDecompression: return "bspatch_decompression";
            case RecipeType.ZlibDecompression: return "zlib_decompression";
            case RecipeType.ZlibCompression: return "zlib_compression";
            case RecipeType.ZstdCompression: return "zstd_compression";
            case RecipeType.ZstdDecompression: return "zstd_decompression";
            }

            throw new Exception($"Unexpected recype type: {type}");
        }

        public Recipe(RecipeType recipeType, ItemDefinition result, List<UInt64> numbers, List<ItemDefinition> items)
            : this(RecipeTypeToString(recipeType), result, numbers, items)
        {
        }

        public Recipe(string name, ItemDefinition result, List<UInt64> numbers, List<ItemDefinition> items)
        {
            if (items.Any(x => x.Equals(result)))
            {
                throw new Exception($"Creating self referential recipe! Name: {name}, Result: {result}");
            }

            Name = name;
            Result = result;
            NumberIngredients = numbers;
            ItemIngredients = items;
        }

        public override int GetHashCode()
        {
            if (!HashCodeCache.HasValue)
            {
                ComputeHashCode();
            }

            return HashCodeCache.Value;
        }

        private void ComputeHashCode()
        {
            int hashCode = Name.GetHashCode();

            hashCode = HashCode.Combine(hashCode, Result.GetHashCode());

            foreach (var number in NumberIngredients)
            {
                hashCode = HashCode.Combine(hashCode, number.GetHashCode());
            }

            foreach (var item in ItemIngredients)
            {
                hashCode = HashCode.Combine(hashCode, item.GetHashCode());
            }

            HashCodeCache = hashCode;
        }

        public override bool Equals(object obj)
        {
            if (obj is not Recipe)
            {
                return false;
            }

            return GetHashCode() == obj.GetHashCode();
        }
    }
}
