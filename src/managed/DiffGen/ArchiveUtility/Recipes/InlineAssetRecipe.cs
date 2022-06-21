/**
 * @file InlineAssetRecipe.cs
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
    public class InlineAssetRecipe : RecipeBase<InlineAssetRecipe>
    {
        public override string Name => "InlineAsset";
        public override RecipeType Type => RecipeType.InlineAsset;
        protected override List<string> ParameterNames => new();
        public InlineAssetRecipe() : base() { }
    }

    public class InlineAssetCopyRecipe : RecipeBase<InlineAssetCopyRecipe>
    {
        public override string Name => "InlineAssetCopy";
        public override RecipeType Type => RecipeType.InlineAssetCopy;
        protected override List<string> ParameterNames => new() { "Offset" };
        public ulong Offset => GetNumberParameter("Offset");
        public InlineAssetCopyRecipe() : base() { }
        public InlineAssetCopyRecipe(ulong offset) : base(new NumberRecipeParameter(offset)) { }
    }
}
