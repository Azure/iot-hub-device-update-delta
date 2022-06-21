/**
 * @file ZstdCompressionRecipe.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ArchiveUtility
{
    public class ZstdCompressionRecipe : RecipeBase<ZstdCompressionRecipe>
    {
        public override string Name => "ZstdCompression";
        public override RecipeType Type => RecipeType.ZstdCompression;
        protected override List<string> ParameterNames => new() { "Item", "MajorVersion", "MinorVersion", "Level" };
        public ArchiveItem Item => GetArchiveItemParameter("Item");
        public ulong MajorVersion => GetNumberParameter("MajorVersion");
        public ulong MinorVersion => GetNumberParameter("MinorVersion");
        public ulong Level => GetNumberParameter("Level");
        public ZstdCompressionRecipe() : base() { }
        public ZstdCompressionRecipe(ArchiveItem item, UInt64 majorVersion, UInt64 minorVersion, UInt64 level)
            : base(new ArchiveItemRecipeParameter(item), new NumberRecipeParameter(majorVersion), new NumberRecipeParameter(minorVersion), new NumberRecipeParameter(level))
        {
        }
        public override bool IsCompressedDependency(ArchiveItem dependency)
        {
            return Item.Equals(dependency) || BinaryData.CompareHashes(dependency.Hashes, Item.Hashes);
        }
    }
}
