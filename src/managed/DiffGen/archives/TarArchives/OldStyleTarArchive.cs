/**
 * @file OldStyleTarArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace TarArchives
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;

    using ArchiveUtility;

    // The format for tar used this link: https://www.systutorials.com/docs/linux/man/5-tar/
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class OldStyleTarArchive : TarArchiveBase
    {
        public OldStyleTarArchive(ArchiveLoaderContext context)
            : base(context)
        {
        }

        public override string ArchiveSubtype { get => "old"; }

        private const int HeaderSize = 512;

        protected override HeaderDetails ReadHeader(BinaryReader reader)
        {
            byte[] rawHeaderData = new byte[HeaderSize];
            if (reader.Read(rawHeaderData, 0, HeaderSize) != HeaderSize)
            {
                throw new FormatException($"Couldn't find enough bytes for header (required at least {HeaderSize} for Old Style Tar.");
            }

            if (AsciiData.IsAllNul(rawHeaderData))
            {
                var allZeroHeaderItem = ItemDefinition.FromByteSpan(rawHeaderData);
                List<UInt64> numbers = new() { allZeroHeaderItem.Length };
                List<ItemDefinition> items = new();
                var headerRecipe = new Recipe(RecipeType.AllZeros, allZeroHeaderItem, numbers, items);
                EmptyBlocks++;
                return new HeaderDetails(allZeroHeaderItem, headerRecipe, null, 0);
            }

            EmptyBlocks = 0;

            var name = AsciiData.FromNulPaddedString(rawHeaderData, 0, 100);
            var mode = AsciiData.FromOctalData(rawHeaderData, 100, 8);
            var uid = AsciiData.FromOctalData(rawHeaderData, 108, 8);
            var gid = AsciiData.FromOctalData(rawHeaderData, 116, 8);
            var size = AsciiData.FromOctalData(rawHeaderData, 124, 12);
            var mtime = AsciiData.FromOctalData(rawHeaderData, 136, 12);
            var checksum = AsciiData.FromOctalData(rawHeaderData, 148, 8);
            var linkflag = AsciiData.FromOctalData(rawHeaderData, 156, 1);
            var linkname = AsciiData.FromNulPaddedString(rawHeaderData, 157, 100);

            var chunkName = ChunkNames.MakeHeaderChunkName(name);
            var headerItem = ItemDefinition.FromByteSpan(rawHeaderData).WithName(chunkName);

            return new HeaderDetails(headerItem, null, name, size);
        }
    }
}
