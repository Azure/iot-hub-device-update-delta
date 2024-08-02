/**
 * @file SWUpdateArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace SWUpdateArchives
{
    using System;

    using ArchiveUtility;
    using Microsoft.Extensions.Logging;

    public class SWUpdateArchive : ArchiveImpl
    {
        private const string SWUpdateDescriptionFile = "sw-description";
        private const string SWUpdateDescriptionSigFile = "sw-description.sig";

        public SWUpdateArchive(ArchiveLoaderContext context)
            : base(context)
        {
        }

        public override string ArchiveType { get => "SWUpdate"; }

        public override string ArchiveSubtype { get => "base"; }

        public override bool TryTokenize(ItemDefinition archiveItem, out ArchiveTokenization tokens)
        {
            if (!ArchiveLoader.TryLoadArchive(Context, out ArchiveTokenization parentTokens, "cpio", "tar"))
            {
                Context.Logger.LogInformation("SWUpdate files are based on either 'cpio' or 'tar' format archives.");
                tokens = null;
                return false;
            }

            // We may later want to have the subtype contain SWUpdate version information, but for now
            // the parent archive type is relavant to us, but the SWUpdate version is not
            ArchiveTokenization swuTokens = new("SWUpdate", parentTokens.Type + "." + parentTokens.Subtype, parentTokens);
            swuTokens.ArchiveItem = archiveItem;

            if (!swuTokens.HasRootPayload(SWUpdateDescriptionFile))
            {
                tokens = null;
                return false;
            }

            tokens = swuTokens;
            return true;
        }

        public override void PostProcess(ArchiveTokenization tokens, ArchiveLoaderContext context)
        {
            // The parent archive (tar or cpio) will have already been post processed
        }
    }
}
