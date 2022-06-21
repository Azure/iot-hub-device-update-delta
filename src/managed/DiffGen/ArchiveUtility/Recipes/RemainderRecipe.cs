/**
 * @file RemainderRecipe.cs
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
    public class RemainderRecipe : RecipeBase<RemainderRecipe>
    {
        public override string Name => "Remainder";
        public override RecipeType Type => RecipeType.Remainder;
        protected override List<string> ParameterNames => new() { };
        public RemainderRecipe() : base() { }
    }
}
