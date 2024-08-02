/**
 * @file AsciiCpioArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace CpioArchives
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Text;
    using ArchiveUtility;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class AsciiCpioArchive : CpioArchiveBase
    {
        public const int AsciiHeaderSize = 76;

        public const string AsciiMagic = "070707";

        public override string ArchiveSubtype
        {
            get { return "ascii.old"; }
        }

        public AsciiCpioArchive(ArchiveLoaderContext context)
            : base(context)
        {
        }

        protected override HeaderDetails ReadHeader(BinaryReader reader)
        {
            byte[] rawHeaderData = new byte[AsciiHeaderSize];
            if (reader.Read(rawHeaderData, 0, AsciiHeaderSize) != AsciiHeaderSize)
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
            if (namesize != (UInt64)reader.Read(rawName, 0, (int)namesize))
            {
                throw new FormatException("Not enough data for the name for CPIO ascii header");
            }

            var name = AsciiData.FromNulPaddedString(rawName, 0, (int)namesize);

            var allData = new List<byte>();
            allData.AddRange(rawHeaderData);
            allData.AddRange(rawName);

            int allDataSize = allData.Count;

            if ((name.CompareTo(TrailerFileName) == 0) && (filesize == 0) && (uid == 0))
            {
                TrailerDetected = true;
            }

            var chunkName = ChunkNames.MakeHeaderChunkName(name);
            var headerItem = ItemDefinition.FromByteSpan(allData.ToArray()).WithName(chunkName);

            return new HeaderDetails(headerItem, null, name, filesize);
        }

        protected override UInt64 AlignmentPadding
        {
            get { return 0; }
        }
    }
}
