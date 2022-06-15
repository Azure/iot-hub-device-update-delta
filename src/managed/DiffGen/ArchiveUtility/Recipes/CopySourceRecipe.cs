/**
 * @file CopySourceRecipe.cs
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
    // We don't need to save the length, because this can be determined from the
    // parent recipe.
    public class CopySourceRecipe : RecipeBase<CopySourceRecipe>
    {
        public override string Name => "CopySource";
        public override RecipeType Type => RecipeType.CopySource;
        protected override List<string> ParameterNames => new() { "Offset" };
        public ulong Offset => GetNumberParameter("Offset");

        public CopySourceRecipe() : base() { }
        public CopySourceRecipe(ulong offset) : base(new NumberRecipeParameter(offset)) { }
    }
}
