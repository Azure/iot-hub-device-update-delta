/**
 * @file AddCopiesForDuplicateChunks.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System.Collections.Generic;
    using System.Security.Authentication;
    using Microsoft.Extensions.Logging;

    using ArchiveUtility;
    using System.Threading;

    /// <summary>
    /// This worker will inspect all of the chunks in our diff and look for entires with identical hashes.
    /// When an identical hash is found we can set the recipe for the second chunk to simply copy the original.
    /// Imagine each letter refers to the contents of a chunk: ABCDEFAFEIASJIAOPE
    /// When we exmaine the chunks we can see multiple occurrences of "A", so the later ones can simply
    /// copy whatever "A" has generated to get the proper content. This can be done for any content that is repeated.
    /// </summary>
    class AddCopiesForDuplicateChunks : Worker
    {
        public Diff Diff { get; set; }
        public int ChunkCountCopiedFromTarget { get; set; }
        public ulong TotalBytesCopiedFromTarget { get; set; }

        public AddCopiesForDuplicateChunks(CancellationToken cancellationToken):
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            Logger.LogInformation("Adding copies for repeated target chunks.");

            Dictionary<string, ArchiveItem> chunksByHash = new();

            foreach (var chunk in Diff.Chunks)
            {
                CheckForCancellation();

                string hash = chunk.Hashes[HashAlgorithmType.Sha256];

                if (!chunksByHash.ContainsKey(hash))
                {
                    chunksByHash[hash] = chunk;
                    continue;
                }

                var firstChunk = chunksByHash[hash];
                chunk.Recipes.Clear();
                var recipe = new CopyRecipe(firstChunk.MakeReference());
                chunk.Recipes.Add(recipe);
                TotalBytesCopiedFromTarget += chunk.Length;
                ChunkCountCopiedFromTarget++;
            }
        }
    }
}
