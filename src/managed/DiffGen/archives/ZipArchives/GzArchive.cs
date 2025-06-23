namespace ZipArchives
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.IO.Compression;
    using System.Text;

    using ArchiveUtility;
    using Microsoft.Extensions.Logging;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class GzArchive : ArchiveImpl
    {
        public GzArchive(ArchiveLoaderContext context)
            : base(context)
        {
        }

        public override string ArchiveType => "zip";

        public override string ArchiveSubtype => "gz";

        private const ulong ZlibGzInitType = 1;

        private bool TryDetermineGzRecipe(ItemDefinition unzippedItem, string unzippedPath, ItemDefinition zippedItem, string zippedPath, out Recipe gzRecipe)
        {
            var result = DiffApi.NativeMethods.diff_get_zlib_compression_level(unzippedPath, zippedPath, out int compressionLevel);

            if (result != 0)
            {
                gzRecipe = null;
                return false;
            }

            ulong compressionLevelUlong = (ulong)compressionLevel;

            if (compressionLevel == -1)
            {
                compressionLevelUlong = uint.MaxValue;
            }

            gzRecipe = new Recipe(RecipeType.ZlibCompression, zippedItem, new() { ZlibGzInitType, compressionLevelUlong }, new() { unzippedItem });
            return true;
        }

        public override bool TryTokenize(ItemDefinition archiveItem, out ArchiveTokenization tokens)
        {
            // Don't bother doing this for very small zips
            if (archiveItem.Length < 10 * 1024 * 1024)
            {
                tokens = null;
                return false;
            }

            ItemDefinition zippedItem = GetItemFromStream(Context.Stream);

            if (!string.IsNullOrEmpty(Context.OriginalArchiveFileName))
            {
                zippedItem = zippedItem.WithName(Context.OriginalArchiveFileName);
            }

            var newTokens = new ArchiveTokenization("zip", "gz");
            newTokens.ArchiveItem = zippedItem;

            string unzippedPath;
            ItemDefinition unzippedItem;

            try
            {
                if (Context.UseCase == ArchiveUseCase.DiffSource)
                {
                    unzippedItem = GetUnzippedItemFromStream(Context.Stream);
                }
                else
                {
                    unzippedPath = zippedItem.GetExtractionPath(Context.WorkingFolder) + ".unzipped";
                    if (!File.Exists(unzippedPath))
                    {
                        var tmpPath = unzippedPath + ".tmp";
                        UnzipToFile(Context.Stream, tmpPath);
                        File.Move(tmpPath, unzippedPath, true);
                    }

                    unzippedItem = ItemDefinition.FromFile(unzippedPath);
                    using FileFromStream zippedFile = new FileFromStream(Context.Stream, Context.WorkingFolder);

                    if (TryDetermineGzRecipe(unzippedItem, unzippedPath, zippedItem, zippedFile.Name, out var gzRecipe))
                    {
                        newTokens.AddForwardRecipe(gzRecipe);
                    }
                    else
                    {
                        Context.Logger?.LogWarning("Couldn't determine compression recipe for item: {unzippedItem}", unzippedItem);
                    }
                }
            }
            catch (Exception e)
            {
                if (Context.Logger != null)
                {
                    Context.Logger.Log(Context.LogLevel, $"[Failed to load stream as GZip. Error: {e.Message}]");
                }

                tokens = null;
                return false;
            }

            Recipe unzipRecipe = new(RecipeType.ZlibDecompression, unzippedItem, new() { ZlibGzInitType }, new() { zippedItem });
            newTokens.AddReverseRecipe(unzipRecipe);

            if (!string.IsNullOrEmpty(Context.OriginalArchiveFileName))
            {
                string uncompressedFileName = Path.GetFileNameWithoutExtension(Context.OriginalArchiveFileName);

                newTokens.AddRootPayload(uncompressedFileName, unzippedItem);
            }

            tokens = newTokens;
            return true;
        }

        private ItemDefinition GetItemFromStream(Stream stream)
        {
            stream.Seek(0, SeekOrigin.Begin);
            using BinaryReader reader = new(stream, Encoding.UTF8, leaveOpen: true);
            ulong length = (ulong)Context.Stream.Length;
            return ItemDefinition.FromBinaryReader(reader, length);
        }

        private void UnzipToFile(Stream stream, string fileName)
        {
            stream.Seek(0, SeekOrigin.Begin);
            using GZipStream gzipStream = new(stream, CompressionMode.Decompress, leaveOpen: true);

            using var fileWriter = File.Create(fileName);
            gzipStream.CopyTo(fileWriter);
        }

        private ItemDefinition GetUnzippedItemFromStream(Stream stream)
        {
            stream.Seek(0, SeekOrigin.Begin);
            using GZipStream gzipStream = new(stream, CompressionMode.Decompress, leaveOpen: true);
            using MemoryStream memStream = new MemoryStream();
            Context.Stream.Seek(0, SeekOrigin.Begin);
            gzipStream.CopyTo(memStream);

            memStream.Seek(0, SeekOrigin.Begin);
            using BinaryReader unzippedReader = new(memStream);
            ulong unzippedLength = (ulong)memStream.Length;
            return ItemDefinition.FromBinaryReader(unzippedReader, unzippedLength);
        }
    }
}
