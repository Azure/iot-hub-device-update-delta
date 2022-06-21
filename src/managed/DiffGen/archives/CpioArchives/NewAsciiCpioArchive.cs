/**
 * @file NewAsciiCpioArchive.cs
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
    public class NewAsciiCpioArchive : CpioArchiveBase
    {
        public const int NewAsciiFormatHeaderSize = 110;
        public const string NewAsciiMagic = "070701";
        public const string NewcAsciiMagic = "070702";

        private string subtype;
        public override string ArchiveSubtype { get { return subtype; } }
        public NewAsciiCpioArchive(ArchiveLoaderContext context) : base(context)
        {
        }

        protected override void ReadHeader(BinaryReader reader, UInt64 offset, out ArchiveItem chunk, out string payloadName, out UInt64 payloadLength)
        {
            byte[] rawHeaderData = new byte[NewAsciiFormatHeaderSize];
            if (NewAsciiFormatHeaderSize != reader.Read(rawHeaderData, 0, NewAsciiFormatHeaderSize))
            {
                throw new FormatException("Not enough data for CPIO new ascii header");
            }

            var magicBytes = new ReadOnlySpan<byte>(rawHeaderData, 0, 6);
            string magic = Encoding.ASCII.GetString(magicBytes);

            if (magic.CompareTo(NewAsciiMagic) == 0)
            {
                subtype = "ascii.new";
            }
            else if (magic.CompareTo(NewcAsciiMagic) == 0)
            {
                subtype = "ascii.newc";
            }
            else
            {
                throw new FormatException($"Found \"{magic}\" instead of expected new ascii header \"{NewAsciiMagic}\" or newc header \"{NewcAsciiMagic}\"");
            }

            var ino = AsciiData.FromHexadecimalData(rawHeaderData, 6, 8);
            var mode = AsciiData.FromHexadecimalData(rawHeaderData, 14, 8);
            var uid = AsciiData.FromHexadecimalData(rawHeaderData, 22, 8);
            var gid = AsciiData.FromHexadecimalData(rawHeaderData, 30, 8);
            var nlink = AsciiData.FromHexadecimalData(rawHeaderData, 38, 8);
            var mtime = AsciiData.FromHexadecimalData(rawHeaderData, 46, 8);
            var filesize = AsciiData.FromHexadecimalData(rawHeaderData, 54, 8);
            var devMajor = AsciiData.FromHexadecimalData(rawHeaderData, 62, 8);
            var devMinor = AsciiData.FromHexadecimalData(rawHeaderData, 70, 8);
            var rdevMajor = AsciiData.FromHexadecimalData(rawHeaderData, 78, 8);
            var rdevMinor = AsciiData.FromHexadecimalData(rawHeaderData, 86, 8);
            var namesize = AsciiData.FromHexadecimalData(rawHeaderData, 94, 8);
            var check = AsciiData.FromHexadecimalData(rawHeaderData, 102, 8);

            // CPIO appends an extra NUL character if the namesize is odd
            int rawNameSize = (int)namesize;
            if ((rawNameSize & 1) == 1)
            {
                rawNameSize++;
            }
            byte[] rawName = new byte[rawNameSize];
            if (rawNameSize != reader.Read(rawName, 0, rawNameSize))
            {
                throw new FormatException("Not enough data for the name for CPIO new ascii header");
            }

            var name = AsciiData.FromNulPaddedString(rawName, 0, rawNameSize);

            var allData = new List<byte>();
            allData.AddRange(rawHeaderData);
            allData.AddRange(rawName);

            UInt64 allDataSize = (UInt64) allData.Count;

            UInt64 paddingNeeded = BinaryData.GetPaddingNeeded(allDataSize, 4);

            if (paddingNeeded != 0)
            {
                byte[] padding = new byte[paddingNeeded];
                if (paddingNeeded != (UInt64) reader.Read(padding, 0, (int) paddingNeeded))
                {
                    throw new FormatException("Couldn't read padding for header.");
                }
                allData.AddRange(padding);
                allDataSize += paddingNeeded;
            }

            var chunkName = ArchiveItem.MakeHeaderChunkName(name);
            chunk = ArchiveItem.FromByteSpan(chunkName, ArchiveItemType.Chunk, allData.ToArray(), (UInt64)offset);
            payloadName = name;
            payloadLength = filesize;

            if ((name.CompareTo(TrailerFileName) == 0) && (filesize == 0) && (uid == 0))
            {
                TrailerDetected = true;
            }
        }

        protected override UInt64 AlignmentPadding { get { return 4; } }
    }
}
