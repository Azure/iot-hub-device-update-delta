/**
 * @file DeltaRecipeBase.cs
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
    public abstract class DeltaRecipeBase<T> : DeltaRecipe where T : Recipe, new()
    {
        public DeltaRecipeBase() : base() { }
        public DeltaRecipeBase(params RecipeParameter[] recipeParameters) : base(recipeParameters) { }
        public DeltaRecipeBase(params ArchiveItem[] archiveItems) : base(archiveItems) { }
        public override Recipe DeepCopy()
        {
            return DeepCopy<T>(this);
        }
    }
}
