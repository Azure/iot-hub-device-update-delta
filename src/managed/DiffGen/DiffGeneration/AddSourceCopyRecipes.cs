/**
 * @file AddSourceCopyRecipes.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System.Collections.Generic;
    using System.Security.Authentication;
    using System.Threading;
    using Microsoft.Extensions.Logging;

    using ArchiveUtility;
    class AddSourceCopyRecipes : Worker
    {
        public Diff Diff { get; set; }
        public ArchiveTokenization SourceTokens { get; set; }
        public bool UseSourceFile { get; set; }
        public ulong TotalBytesCopiedFromSourceChunks { get; private set; }
        public int ChunkCountCopiedFromSourceChunks { get; private set; }

        public AddSourceCopyRecipes(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            if (!UseSourceFile)
            {
                Logger.LogInformation("UseSourceFile is false - nothing to do.");
            }
            Dictionary<string, ArchiveItem> sourceChunkLookup = new();

            foreach (var sourceChunk in SourceTokens.Chunks)
            {
                var hash = sourceChunk.Hashes[HashAlgorithmType.Sha256];
                sourceChunkLookup[hash] = sourceChunk;
            }

            foreach (var chunk in Diff.Chunks)
            {
                CheckForCancellation();
                if (chunk.Recipes.Count != 0)
                {
                    continue;
                }

                var hash = chunk.Hashes[HashAlgorithmType.Sha256];
                if (!sourceChunkLookup.ContainsKey(hash))
                {
                    continue;
                }

                var sourceChunk = sourceChunkLookup[hash];
                var recipe = new CopySourceRecipe(sourceChunk.Offset.Value);

                TotalBytesCopiedFromSourceChunks += sourceChunk.Length;
                ChunkCountCopiedFromSourceChunks++;

                chunk.Recipes.Add(recipe);
            }
        }
    }
}
