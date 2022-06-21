/**
 * @file ApplyZstdDeltaRecipe.cs
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
    public class ApplyZstdDeltaRecipe : DeltaRecipeBase<ApplyZstdDeltaRecipe>
    {
        public override string Name => "ApplyZstdDelta";
        public override RecipeType Type => RecipeType.ApplyZstdDelta;
        public ApplyZstdDeltaRecipe() : base() { }
        public ApplyZstdDeltaRecipe(params RecipeParameter[] recipeParameters) : base(recipeParameters) { }
        public ApplyZstdDeltaRecipe(params ArchiveItem[] archiveItems) : base(archiveItems) { }
    }
}
