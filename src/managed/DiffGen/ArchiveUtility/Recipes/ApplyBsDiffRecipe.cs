/**
 * @file ApplyBsDiffRecipe.cs
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
    public class ApplyBsDiffRecipe : DeltaRecipeBase<ApplyBsDiffRecipe>
    {
        public override string Name => "ApplyBsDiff";
        public override RecipeType Type => RecipeType.ApplyBsDiff;
        public ApplyBsDiffRecipe() : base() { }
        public ApplyBsDiffRecipe(params RecipeParameter[] recipeParameters) : base(recipeParameters) { }
        public ApplyBsDiffRecipe(params ArchiveItem[] archiveItems) : base(archiveItems) { }
    }
}
