/**
 * @file FinalizeInlineAssets.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Security.Authentication;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    using ArchiveUtility;

    internal class FinalizeInlineAssets : Worker
    {
        public Diff Diff { get; set; }

        public FinalizeInlineAssets(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            var allInlineAssetItems = Diff.GetAllInlineAssetArchiveItems();

            Dictionary<string, UInt64> offsetOfInlineAsset = new();

            UInt64 currentOffset = 0;
            foreach (var item in allInlineAssetItems)
            {
                CheckForCancellation();
                var hash = item.Hashes[HashAlgorithmType.Sha256];

                if (!offsetOfInlineAsset.ContainsKey(hash))
                {
                    offsetOfInlineAsset[hash] = currentOffset;
                    currentOffset += item.Length;
                    continue;
                }

                var offset = offsetOfInlineAsset[hash];

                item.Recipes[0] = new InlineAssetCopyRecipe(offset);
            }
        }
    }
}
