/**
 * @file AddDeltaBasedRecipes.cs
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
    using System.Threading;
    using Microsoft.Extensions.Logging;

    using ArchiveUtility;

    /// <summary>
    /// This worker will look for payload in the source can be transformed into payload used in 
    /// recipes on the target.
    /// The trivial example here is a chunk that is exactly equal to a payload in the target.
    /// Let's call the chunk C(T) and payload P(T). If we can find a payload in the source, P(S)
    /// that matches the P(T) then we can use a delta transformation on P(S) to generate P(T) and
    /// then  C(T), because C(T) is a copy of P(T).
    /// A more complicated case would involve a non-copy recipe of P(T) to generate C(T), but this still
    /// works. Here we'd have P(T) = D(P(S)->P(T)), which we can substitue for P(T) and still get a recipe
    /// that will generate C(T).
    /// Suppose C(T) is a region of P(T), bytes 1000 to 5000. Its recipe would look like
    /// Region(1000, 5000, P(T)).
    /// If we know that the recipe for P(S) and a delta from P(S)->P(T) as D(P(S)->P(T)), 
    /// we can subtitute it in for P(T) here and 
    /// get a new recipe that looks like Region(1000, 5000, D(P(S)->P(T))).
    /// If the recipe for P(S) contains any copies from the source archive, we translate them into
    /// CopySource recipes so we will look in the right location.
    /// </summary>
    class AddDeltaBasedRecipes : Worker
    {
        public Diff Diff { get; set; }
        public ArchiveTokenization SourceTokens { get; set; }
        public ArchiveTokenization TargetTokens { get; set; }
        public ArchiveTokenization DeltaTokens { get; set; }
        public bool UsePayloadDevicePaths { get; set; }
        public bool UseSourceFile { get; set; }
        public ulong TotalBytesFromDeltaBasedRecipes{ get; private set; }
        public int ChunkCountFromDeltaBasedRecipes { get; private set; }

        public AddDeltaBasedRecipes(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected bool SearchForRecipesUsingDeltaPayload(ArchiveItem chunk, Dictionary<string, ArchiveItem> deltaPayloadByHash)
        {
            var targetChunk = TargetTokens.ArchiveItems.Get(chunk.Id.GetValueOrDefault());

            // TODO: Add ability to use multiple different payloads that have deltas if available.
            foreach (var recipe in targetChunk.Recipes)
            {
                CheckForCancellation();

                var dependencies = recipe.GetDependencies().Where(d => d.Type != ArchiveItemType.Blob).ToList();

                if (dependencies.Count != 1)
                {
                    continue;
                }

                var dependency = dependencies[0];
                var dependencyHash = dependency.Hashes[HashAlgorithmType.Sha256];

                if (deltaPayloadByHash.ContainsKey(dependencyHash))
                {
                    var deltaPayload = deltaPayloadByHash[dependencyHash];

                    if (UseSourceFile)
                    {
                        // Get a delta-based version of the payload instead including an invocation
                        // to some delta algorithm instead of using chunks.
                        var replacedRecipe = recipe.ReplaceItem(dependency, deltaPayload);
                        // Translate references to chunks in the source archive into "CopySource"
                        // which equates to a copy from the source archive during diff application
                        // with the appropriate offset + length.
                        var newRecipe = replacedRecipe.Export();
                        chunk.Recipes.Add(newRecipe);
                    }
                    else
                    {
                        // TODO: Test and implement using payload from disk
                        chunk.Recipes.Add(Recipe.ReplaceItem(recipe, dependency, deltaPayload.MakeReference()));
                    }
                }

                return true;
            }

            return false;
        }

        protected bool SearchForRecipesUsingDeltaChunks(ArchiveItem chunk, Dictionary<string, ArchiveItem> deltaChunksByHash)
        {
            var hash = chunk.Hashes[HashAlgorithmType.Sha256];

            if (!deltaChunksByHash.ContainsKey(hash))
            {
                return false;
            }

            var deltaChunk = deltaChunksByHash[hash];

            if (deltaChunk.Recipes.Count == 0)
            {
                return false;
            }

            var recipe = deltaChunk.Recipes[0];
            // Translate references to chunks in the source archive into "CopySource"
            // which equates to a copy from the source archive during diff application
            // with the appropriate offset + length.
            var newRecipe = recipe.Export();
            chunk.Recipes.Add(newRecipe);

            return true;
        }

        protected override void ExecuteInternal()
        {
            if (!UsePayloadDevicePaths && !UseSourceFile)
            {
                Logger.LogInformation("UsePayloadDevicePaths and UseSourceFile are false - nothing to do.");
                return;
            }

            Func<IEnumerable<ArchiveItem>, Dictionary<string, ArchiveItem>> makeDeltaLookup = (values) => 
            {
                Dictionary<string, ArchiveItem> dictionary = new();
                foreach (var value in values)
                {
                    var hash = value.Hashes[HashAlgorithmType.Sha256];
                    dictionary[hash] = value;
                }

                return dictionary;
            };

            var deltaPayloadByHash = makeDeltaLookup(DeltaTokens.Payload);
            var deltaChunksByHash = makeDeltaLookup(DeltaTokens.Chunks);

            var chunks = Diff.Chunks.Where(c => c.Recipes.Count == 0);
            foreach (var chunk in chunks)
            {
                CheckForCancellation();

                if (SearchForRecipesUsingDeltaPayload(chunk, deltaPayloadByHash))
                {
                    TotalBytesFromDeltaBasedRecipes += chunk.Length;
                    ChunkCountFromDeltaBasedRecipes++;
                    continue;
                }

                if (!UseSourceFile)
                {
                    continue;
                }

                if (SearchForRecipesUsingDeltaChunks(chunk, deltaChunksByHash))
                {
                    TotalBytesFromDeltaBasedRecipes += chunk.Length;
                    ChunkCountFromDeltaBasedRecipes++;
                    continue;
                }
            }
        }
    }
}
