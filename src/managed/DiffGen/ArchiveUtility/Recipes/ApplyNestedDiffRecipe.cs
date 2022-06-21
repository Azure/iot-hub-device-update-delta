/**
 * @file ApplyNestedDiffRecipe.cs
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
    public class ApplyNestedDiffRecipe : DeltaRecipeBase<ApplyNestedDiffRecipe>
    {
        public override string Name => "ApplyNestedDiff";
        public override RecipeType Type => RecipeType.ApplyNestedDiff;
        public ApplyNestedDiffRecipe() : base() { }
        public ApplyNestedDiffRecipe(params RecipeParameter[] recipeParameters) : base(recipeParameters) { }
        public ApplyNestedDiffRecipe(params ArchiveItem[] archiveItems) : base(archiveItems) { }
    }
}
