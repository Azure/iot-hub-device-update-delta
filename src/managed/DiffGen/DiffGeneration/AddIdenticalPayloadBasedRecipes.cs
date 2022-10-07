/**
 * @file AddIdenticalPayloadBasedRecipes.cs
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

    /// <summary>
    /// This worker will look for payload in the source that matches payload used in recipes on the target.
    /// The trivial example here is a chunk that is exactly equal to a payload in the target.
    /// Let's call the chunk C(T) and payload P(T). If we can find a payload in the source, P(S)
    /// that matches the P(T) then we can use whatever recipe is in the P(S) to generate P(T) and
    /// also C(T), because C(T) is a copy of P(T).
    /// A more complicated case would involve a non-copy recipe of P(T) to generate C(T), but this still
    /// works, because we can substitute the recipe for P(S) for P(T) and still get a recipe
    /// that will generate C(T).
    /// Suppose C(T) is a region of P(T), bytes 1000 to 5000. Its recipe would look like
    /// Region(1000, 5000, P(T)).
    /// If we know that the recipe for P(S), we can subtitute it in for P(T) here and 
    /// get a new recipe that looks like Region(1000, 5000, P(S)).
    /// If the recipe for P(S) contains any copies from the source archive, we translate them into
    /// CopySource recipes so we will look in the right location.
    /// </summary>
    class AddIdenticalPayloadBasedRecipes : Worker
    {
        public Diff Diff { get; set; }
        public ArchiveTokenization SourceTokens { get; set; }
        public ArchiveTokenization TargetTokens { get; set; }
        public bool UsePayloadDevicePaths { get; set; }
        public bool UseSourceFile { get; set; }
        public ulong TotalBytesCopiedFromIdenticalPayload { get; private set; }
        public int ChunkCountCopiedFromIdenticalPayload { get; private set; }

        public AddIdenticalPayloadBasedRecipes(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            if (!UsePayloadDevicePaths && !UseSourceFile)
            {
                Logger.LogInformation("UsePayloadDevicePaths and UseSourceFile are false - nothing to do.");
                return;
            }

            Dictionary<string, ArchiveItem> sourcePayloadLookup = new();

            foreach (var sourcePayload in SourceTokens.Payload)
            {
                var hash = sourcePayload.Hashes[HashAlgorithmType.Sha256];
                sourcePayloadLookup[hash] = sourcePayload;
            }

            // Create a mapping between target and diff chunks. The indexes don't match
            // exactly because the target tokens may contain empty chunks, while
            // these are omitted from the diff tokens
            Dictionary<ulong, ArchiveItem> targetChunkLookupByOffset = new();
            foreach (var chunk in TargetTokens.Chunks)
            {
                targetChunkLookupByOffset[chunk.Offset.Value] = chunk;
            }

            foreach (var chunk in Diff.Chunks)
            {
                CheckForCancellation();

                if (chunk.Recipes.Count != 0)
                {
                    continue;
                }

                // var depenedencies = chunk.Recipes
                var targetChunk = targetChunkLookupByOffset[chunk.Offset.Value];

                foreach (var recipe in targetChunk.Recipes)
                {
                    CheckForCancellation();

                    var dependencies = recipe.GetDependencies();

                    var newRecipe = recipe.DeepCopy();

                    bool mappedAllDependencies = true;

                    foreach (var dependency in dependencies)
                    {
                        var hash = dependency.Hashes[HashAlgorithmType.Sha256];
                        if (!sourcePayloadLookup.ContainsKey(hash))
                        {
                            mappedAllDependencies = false;
                            break;
                        }

                        var payload = sourcePayloadLookup[hash];

                        newRecipe = newRecipe.ReplaceItem(dependency, payload);
                    }

                    if (!mappedAllDependencies)
                    {
                        continue;
                    }

                    newRecipe = newRecipe.Export();
                    chunk.Recipes.Add(newRecipe);

                    TotalBytesCopiedFromIdenticalPayload += chunk.Length;
                    ChunkCountCopiedFromIdenticalPayload++;
                }
            }
        }
    }
}