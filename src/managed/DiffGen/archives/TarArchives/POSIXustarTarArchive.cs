/**
 * @file POSIXustarTarArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace TarArchives
{
    using System.IO;
    using ArchiveUtility;

    // The format for tar used this link: https://www.systutorials.com/docs/linux/man/5-tar/
    public class POSIXustarTarArchive : TarArchiveBase
    {
        public POSIXustarTarArchive(ArchiveLoaderContext context)
            : base(context)
        {
        }

        public override string ArchiveSubtype { get => "ustar"; }

        protected override HeaderDetails ReadHeader(BinaryReader reader)
        {
            return new HeaderDetails();
        }
    }
}