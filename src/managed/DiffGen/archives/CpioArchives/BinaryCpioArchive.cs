/**
 * @file BinaryCpioArchive.cs
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

namespace CpioArchives
{
    public class BinaryCpioArchive : CpioArchiveBase
    {
        public BinaryCpioArchive(ArchiveLoaderContext context) : base(context)
        {

        }
        public override string ArchiveSubtype { get { return "binary"; } }

        const int CpioBinaryHeaderSize = 26;
        const uint BinaryMagic = 0x71C7; // 070707 in octal
        protected override void ReadHeader(BinaryReader reader, UInt64 offset, out ArchiveItem chunk, out string payloadName, out UInt64 payloadLength)
        {
            byte[] rawData = new byte[CpioBinaryHeaderSize];
            if (CpioBinaryHeaderSize != reader.Read(rawData, 0, CpioBinaryHeaderSize))
            {
                throw new FormatException("Not enough data present to store a CPIO binary header");
            }

            var magic = (uint)rawData[0] + 0x100u * (uint)rawData[1];

            if (magic != BinaryMagic)
            {
                throw new FormatException("Magic constant not detected.");
            }

            var dev = rawData[2] + 0x100u * rawData[3];
            var ino = rawData[4] + 0x100u * rawData[5];
            var mode = rawData[6] + 0x100u * rawData[7];
            var uid = rawData[8] + 0x100u * rawData[9];
            var gid = rawData[10] + 0x100u * rawData[11];
            var nlink = rawData[12] + 0x100u * rawData[13];
            var rdev = rawData[14] + 0x100u * rawData[15];
            var mtime = rawData[18] + 0x100u * rawData[19] + 0x10000u * rawData[16] + 0x1000000u * rawData[17];
            var namesize = rawData[20] + 0x100u * rawData[21];
            var filesize = rawData[24] + 0x100u * rawData[25] + 0x10000u * rawData[22] + 0x1000000u * rawData[23];

            // CPIO appends an extra NUL character if the namesize is odd
            int rawNameSize = (int)namesize;
            if ((rawNameSize & 1) == 1)
            {
                rawNameSize++;
            }
            byte[] rawName = new byte[rawNameSize];
            if (rawNameSize != reader.Read(rawName, 0, rawNameSize))
            {
                throw new FormatException("Not enough data for the name for CPIO binary header");
            }

            var name = AsciiData.FromNulPaddedString(rawName, 0, rawNameSize);

            List<byte> allData = new List<byte>();
            allData.AddRange(rawData);
            allData.AddRange(rawName);

            int allDataSize = allData.Count;

            var chunkName = ArchiveItem.MakeHeaderChunkName(name);
            chunk = ArchiveItem.FromByteSpan(chunkName, ArchiveItemType.Chunk, allData.ToArray(), offset);
            payloadName = name;
            payloadLength = filesize;

            if ((name.CompareTo(TrailerFileName) == 0) && (filesize == 0) && (uid == 0))
            {
                TrailerDetected = true;
            }
        }

        protected override UInt64 AlignmentPadding { get { return 2; } }
    }
}
