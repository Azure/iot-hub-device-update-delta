/**
 * @file GzDecompressionRecipe.cs
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
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;

    using System.IO.Compression;

    public class GzDecompressionRecipe : RecipeBase<GzDecompressionRecipe>
    {
        public override string Name => "GzDecompression";
        public override RecipeType Type => RecipeType.GzDecompression;
        protected override List<string> ParameterNames => new() { "Item" };
        public ArchiveItem Item => GetArchiveItemParameter("Item");
        public GzDecompressionRecipe() : base() { }
        public GzDecompressionRecipe(ArchiveItem item) : base(new ArchiveItemRecipeParameter(item))
        {
        }

        public override void CopyTo(Stream archiveStream, Stream writeStream)
        {
            var item = Item;

            if (item == null)
            {
                throw new Exception($"GzDecompressionRecipe.CopyTo(): Item is null. Name: {Name}");
            }

            var compressedFile = Path.GetTempFileName();

            using (var chunkWriteStream = File.Create(compressedFile))
            {
                item.CopyTo(archiveStream, chunkWriteStream);
            }

            using (var compressedStream = File.OpenRead(compressedFile))
            { 
                using var decompressor = new GZipStream(compressedStream, CompressionMode.Decompress);
                decompressor.CopyTo(writeStream);
            }

            File.Delete(compressedFile);
        }
    }
}
