/**
 * @file RegionRecipe.cs
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
    public class RegionRecipe : RecipeBase<RegionRecipe>
    {
        public override string Name => "Region";
        public override RecipeType Type => RecipeType.Region;
        protected override List<string> ParameterNames => new() { "Item", "Offset", "Length" };
        public RegionRecipe() : base() { }
        public RegionRecipe(ArchiveItem blob, UInt64 offset, UInt64 length) :
            base(new ArchiveItemRecipeParameter(blob), new NumberRecipeParameter(offset), new NumberRecipeParameter(length))
        {
        }

        public ArchiveItem Item => GetArchiveItemParameter("Item");

        public ulong Offset => GetNumberParameter("Offset");
        public ulong Length => GetNumberParameter("Length");
    }
}
