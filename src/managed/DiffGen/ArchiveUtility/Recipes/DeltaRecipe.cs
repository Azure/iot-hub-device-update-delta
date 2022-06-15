/**
 * @file DeltaRecipe.cs
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
    public abstract class DeltaRecipe : Recipe
    {
        public DeltaRecipe() : base() { }
        public DeltaRecipe(params RecipeParameter[] recipeParameters) : base(recipeParameters) { }
        public DeltaRecipe(params ArchiveItem[] archiveItems) : base(archiveItems) { }
        protected override List<string> ParameterNames => new() { "Delta", "Source" };
        public ArchiveItem Delta => GetArchiveItemParameter("Delta");
        public ArchiveItem Source => GetArchiveItemParameter("Source");
    }
}
