/**
 * @file AsciiCpioArchive.cs
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
using ArchiveUtility;

namespace CpioArchives
{
    public class AsciiCpioArchive : CpioArchiveBase
    {
        public const int AsciiHeaderSize = 76;
        public const string AsciiMagic = "070707";
        public override string ArchiveSubtype { get { return "ascii.old"; } }

        public AsciiCpioArchive(ArchiveLoaderContext context) : base(context)
        {
        }

        protected override void ReadHeader(BinaryReader reader, UInt64 offset, out ArchiveItem chunk, out string payloadName, out UInt64 payloadLength)
        {
            byte[] rawHeaderData = new byte[AsciiHeaderSize];
            if (AsciiHeaderSize != reader.Read(rawHeaderData, 0, AsciiHeaderSize))
            {
                throw new FormatException("Not enough data for CPIO ascii header");
            }

            var magicBytes = new ReadOnlySpan<byte>(rawHeaderData, 0, 6);
            string magic = Encoding.ASCII.GetString(magicBytes);

            if ((magic.CompareTo(AsciiMagic) != 0) && (magic.CompareTo(AsciiMagic) != 0))
            {
                throw new FormatException($"Found \"{magic}\" instead of expected new ascii header \"{AsciiMagic}\"");
            }

            var dev = AsciiData.FromOctalData(rawHeaderData, 6, 6);
            var ino = AsciiData.FromOctalData(rawHeaderData, 12, 6);
            var mode = AsciiData.FromOctalData(rawHeaderData, 18, 6);
            var uid = AsciiData.FromOctalData(rawHeaderData, 24, 6);
            var gid = AsciiData.FromOctalData(rawHeaderData, 30, 6);
            var nlink = AsciiData.FromOctalData(rawHeaderData, 36, 6);
            var rdev = AsciiData.FromOctalData(rawHeaderData, 42, 6);
            var mtime = AsciiData.FromOctalData(rawHeaderData, 48, 11);
            var namesize = AsciiData.FromOctalData(rawHeaderData, 59, 6);
            var filesize = AsciiData.FromOctalData(rawHeaderData, 65, 11);

            byte[] rawName = new byte[namesize];
            if (namesize != (UInt64) reader.Read(rawName, 0, (int) namesize))
            {
                throw new FormatException("Not enough data for the name for CPIO ascii header");
            }

            var name = AsciiData.FromNulPaddedString(rawName, 0, (int) namesize);

            var allData = new List<byte>();
            allData.AddRange(rawHeaderData);
            allData.AddRange(rawName);

            int allDataSize = allData.Count;

            var chunkName = ArchiveItem.MakeHeaderChunkName(name);
            chunk = ArchiveItem.FromByteSpan(chunkName, ArchiveItemType.Chunk, allData.ToArray(), (UInt64)offset);
            payloadName = name;
            payloadLength = filesize;

            if ((name.CompareTo(TrailerFileName) == 0) && (filesize == 0) && (uid == 0))
            {
                TrailerDetected = true;
            }
        }

        protected override UInt64 AlignmentPadding { get { return 0; } }
    }
}
