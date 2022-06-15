/**
 * @file CopyRecipe.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.IO;

    public class CopyRecipe : RecipeBase<CopyRecipe>
    {
        public override string Name => "Copy";
        public override RecipeType Type => RecipeType.Copy;
        protected override List<string> ParameterNames => new() { "Item" };
        public ArchiveItem Item => GetArchiveItemParameter("Item");
        public CopyRecipe() : base() { }
        public CopyRecipe(params ArchiveItem[] archiveItemParameters) : base(archiveItemParameters) { }

        public override Recipe ReplaceItem(ArchiveItem itemOld, ArchiveItem itemNew)
        {
            if (Item.Equals(itemOld) || (Item.Hashes.Equals(itemOld.Hashes)))
            {
                return itemNew.Recipes[0];
            }

            return base.ReplaceItem(itemOld, itemNew);
        }

        public override void CopyTo(Stream archiveStream, Stream writeStream)
        {
            Item.CopyTo(archiveStream, writeStream);
        }
    }
}
