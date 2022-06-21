/**
 * @file ZstdDecompressionRecipe.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;

    public class ZstdDecompressionRecipe : RecipeBase<ZstdDecompressionRecipe>
    {
        public override string Name => "ZstdDecompression";
        public override RecipeType Type => RecipeType.ZstdDecompression;
        protected override List<string> ParameterNames => new() { "Item" };
        public ArchiveItem Item => GetArchiveItemParameter("Item");
        public ZstdDecompressionRecipe() : base() { }
        public ZstdDecompressionRecipe(ArchiveItem item) : base(new ArchiveItemRecipeParameter(item))
        {
        }

        public static string ZSTD_COMPRESS_FILE_EXE_PATH
        {
            get
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    return "zstd_compress_file";
                }
                
                return "zstd_compress_file.exe";
            }
        }

        const int TEN_MINUTES = 1000 * 60 * 10;

        public override void CopyTo(Stream archiveStream, Stream writeStream)
        {
            var item = Item;

            if (item == null)
            {
                throw new Exception($"ZstdDecompressionRecipe.CopyTo(): Item is null. Name: {Name}");
            }

            var compressedFile = Path.GetTempFileName();

            using (var compressedStream = File.Create(compressedFile))
            {
                item.CopyTo(archiveStream, compressedStream);
            }

            var uncompressedFile = Path.GetTempFileName();

            using (var process = new Process())
            {
                string cmdLine = $"-d \"{compressedFile}\" \"{uncompressedFile}\"";

                process.StartInfo.UseShellExecute = false;
                process.StartInfo.FileName = ProcessHelper.GetPathInRunningDirectory(ZSTD_COMPRESS_FILE_EXE_PATH);
                process.StartInfo.Arguments = cmdLine;

                process.StartAndReport();
                process.WaitForExit(TEN_MINUTES);
            }

            using (var uncompressedStream = File.OpenRead(uncompressedFile))
            {
                uncompressedStream.CopyTo(writeStream);
            }

            File.Delete(uncompressedFile);
        }
    }
}
