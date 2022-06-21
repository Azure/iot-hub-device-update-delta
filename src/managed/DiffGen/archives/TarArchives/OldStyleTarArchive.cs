/**
 * @file OldStyleTarArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using ArchiveUtility;

// The format for tar used this link: https://www.systutorials.com/docs/linux/man/5-tar/

namespace TarArchives
{
    public class OldStyleTarArchive : TarArchiveBase
    {
        public OldStyleTarArchive(ArchiveLoaderContext context) : base(context)
        {
        }
        public override string ArchiveSubtype { get { return "old"; } }
        private const int HeaderSize = 512;
        protected override void ReadHeader(BinaryReader reader, UInt64 offset, out ArchiveItem chunk, out string payloadName, out UInt64 payloadLength)
        {
            byte[] rawHeaderData = new byte[HeaderSize];
            if (HeaderSize != reader.Read(rawHeaderData, 0, HeaderSize))
            {
                throw new FormatException($"Couldn't find enough bytes for header (required at least {HeaderSize} for Old Style Tar.");
            }

            if (AsciiData.IsAllNul(rawHeaderData))
            {
                chunk = ArchiveItem.FromByteSpan(ArchiveItem.MakeAllZeroChunkName(offset), ArchiveItemType.Chunk, rawHeaderData, (UInt64)offset);
                chunk.Recipes.Add(new AllZeroRecipe(chunk.Length));
                payloadName = null;
                payloadLength = 0;
                EmptyBlocks++;
                return;
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

            var chunkName = ArchiveItem.MakeHeaderChunkName(name);
            chunk = ArchiveItem.FromByteSpan(chunkName, ArchiveItemType.Chunk, rawHeaderData, (UInt64)offset);
            payloadName = name;
            payloadLength = size;
        }
    }
}
