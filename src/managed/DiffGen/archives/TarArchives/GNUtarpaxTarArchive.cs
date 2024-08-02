/**
 * @file GNUtarpaxTarArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace TarArchives
{
    using System.IO;
    using ArchiveUtility;

    // The format for tar used this link: https://www.systutorials.com/docs/linux/man/5-tar/
    public class GNUtarpaxTarArchive : TarArchiveBase
    {
        public override string ArchiveSubtype { get => "GNUtarpax"; }

        public GNUtarpaxTarArchive(ArchiveLoaderContext context)
            : base(context)
        {
        }

        protected override HeaderDetails ReadHeader(BinaryReader reader)
        {
            return new HeaderDetails();
        }
    }
}
