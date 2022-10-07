/**
 * @file CreateDiff.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.Extensions.Logging;

    using ArchiveUtility;

    class CreateDiff : Worker
    {
        public string TargetFile { get; set; }
        public ArchiveTokenization TargetTokens { get; set; }
        public string SourceFile { get; set; }
        public Diff Diff { get; private set; }

        public CreateDiff(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            Diff diff = new();

            diff.Version = 0;

            Action getTargetDetails = () =>
            {
                FileInfo targetFileInfo = new(TargetFile);
                diff.TargetSize = (ulong)targetFileInfo.Length;
                diff.TargetHash = Hash.FromFile(TargetFile);

            };
            Action getSourceFileDetails = () =>
            {
                FileInfo sourceFileInfo = new(SourceFile);
                diff.SourceSize = (ulong)sourceFileInfo.Length;
                diff.SourceHash = Hash.FromFile(SourceFile);
            };
            Action populateChunks = () =>
            {
                HashSet<ulong> offsetsFound = new();
                int duplicate = 0;
                int empty = 0;
                foreach (var chunk in TargetTokens.Chunks.OrderBy(c => c.Offset))
                {
                    if (chunk.Length == 0)
                    {
                        empty++;
                        continue;
                    }
                    if (offsetsFound.Contains(chunk.Offset.Value))
                    {
                        duplicate++;
                        continue;
                    }

                    offsetsFound.Add(chunk.Offset.Value);

                    ArchiveItem chunkCopy = chunk.MakeReference();
                    if (chunk.HasAllZeroRecipe())
                    {
                        Recipe azr = chunk.Recipes.Find(r => r is AllZeroRecipe);
                        chunkCopy.Recipes.Add(azr.DeepCopy());
                    }
                    diff.Chunks.Add(chunkCopy);
                }
                Logger.LogInformation($"Skipped {empty} empty chunks and {duplicate} duplicate chunks.");
            };

            var actions = new List<Action>() { getTargetDetails, getSourceFileDetails, populateChunks };
            ParallelOptions po = new() { CancellationToken = CancellationToken };
            CheckForCancellation();
            Parallel.ForEach(actions, po, action => action());

            Diff = diff;
        }
    }
}
