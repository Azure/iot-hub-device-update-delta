/**
 * @file RecipeBase.cs
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
    public abstract class RecipeBase<T> : Recipe where T : Recipe, new()
    {
        public RecipeBase() : base() { }
        public RecipeBase(params ArchiveItem[] items) : base(items)
        {
        }
        public RecipeBase(params RecipeParameter[] recipeParameters) : base(recipeParameters)
        {
        }
        public override Recipe DeepCopy()
        {
            return Recipe.DeepCopy<T>(this);
        }
    }
}
