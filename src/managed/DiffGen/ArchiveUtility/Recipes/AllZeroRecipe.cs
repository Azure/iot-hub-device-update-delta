/**
 * @file AllZeroRecipe.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ArchiveUtility
{
    public class AllZeroRecipe : RecipeBase<AllZeroRecipe>
    {
        public override string Name => "AllZero";
        public override RecipeType Type => RecipeType.AllZero;
        protected override List<string> ParameterNames => new() { "Length" };
        public AllZeroRecipe() : base() { }
        public AllZeroRecipe(UInt64 length) :
            base(new NumberRecipeParameter(length)) { }

        public ulong Length => GetNumberParameter("Length");

        public override void CopyTo(Stream archiveStream, Stream writeStream)
        {
            for (ulong i = 0; i < Length; i++)
            {
                writeStream.WriteByte(0);
            }
        }
    }
}
