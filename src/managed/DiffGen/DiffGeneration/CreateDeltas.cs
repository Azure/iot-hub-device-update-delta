/**
 * @file CreateDeltas.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text.RegularExpressions;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Security.Authentication;

    using Microsoft.Extensions.Logging;

    using ArchiveUtility;

    /// <summary>
    ///  This worker generates deltas that will be used for recipes to regenerate chunks in a diff.
    ///  We skip any chunks that have a recipe. We look at the diff and look for any dependencies 
    ///  that are similar to entries in the source.
    ///  When we find one we use some simple measures to determine if we should create the diff (size)
    ///  and then call into bsdiff to create it.
    ///  The worker will produce an ArchiveTokenization named DeltaTokens with a set of payload
    ///  entries to define the recipes describing the diffs which will be used in subsequent steps.
    /// </summary>
    class CreateDeltas : Worker
    {
        public string SourceFile { get; set; }
        public string TargetFile { get; set; }
        public ArchiveTokenization SourceTokens { get; set; }
        public ArchiveTokenization TargetTokens { get; set; }
        public Diff Diff { get; set; }
        public ArchiveTokenization DeltaTokens { get; set; }
        public string DeltaTokensFile { get; set; }
        public string LogFolder { get; set; }
        public bool UsePayloadDevicePaths { get; set; }
        public bool UseSourceFile { get; set; }
        public bool KeepWorkingFolder { get; set; }

        public CreateDeltas(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            var cookiePath = Path.Combine(WorkingFolder, "deltas.cookie");
            var deltaTokensPath = DiffBuilder.GetDeltaTokensPath(WorkingFolder);
            if (File.Exists(cookiePath) && File.Exists(deltaTokensPath))
            {
                Logger.LogInformation($"Found cookie for deltas at: {cookiePath}. Loading tokens from: {deltaTokensPath}");

                DeltaTokens = ArchiveTokenization.FromJsonPath(deltaTokensPath);
                DeltaTokensFile = deltaTokensPath;
                return;
            }

            DeltaTokens = new("Delta", "LocalFiles");

            // Create a mapping between target and diff chunks. The indexes don't match
            // exactly because the target tokens may contain empty chunks, while
            // these are omitted from the diff tokens
            var diffChunkLookupByOffset = BuildArchiveItemLookupByOffset(Diff.Chunks);

            CheckForCancellation();
            CreatePayloadDeltas(diffChunkLookupByOffset);
            CheckForCancellation();
            CreateChunkDeltas(diffChunkLookupByOffset);

            using (var stream = File.Create(deltaTokensPath))
            {
                DeltaTokens.WriteJson(stream, true);
            }

            DeltaTokensFile = deltaTokensPath;
            CreateCookie(cookiePath);
        }

        protected static bool IsSmallFileSize(long length)
        {
            return length <= 512;
        }

        protected static bool IsCompressedFileName(string name)
        {
            string[] exts = { ".gz", ".xz", ".zstd", ".zip" };

            return exts.Any(ext => name.EndsWith(ext));
        }

        Dictionary<string, Tuple<ArchiveItem, ArchiveItem, bool>> GetDeltasNeeded(ConcurrentDictionary<ulong, ArchiveItem> diffChunkLookupByOffset)
        {
            Dictionary<string, Tuple<ArchiveItem, ArchiveItem, bool>> needed = new();
            var sourcePayloadLookup = BuildArchiveItemLookupByName(SourceTokens.Payload);

            foreach (var targetChunk in TargetTokens.Chunks)
            {
                if (targetChunk.Length == 0)
                {
                    continue;
                }

                if (targetChunk.Recipes.Count == 0)
                {
                    continue;
                }

                // First check if this item is useful for the Diff
                var diffChunk = diffChunkLookupByOffset[targetChunk.Offset.Value];

                // If we already derived a recipe here, then don't do anything else
                if (diffChunk.Recipes.Count != 0)
                {
                    continue;
                }

                // We examine all of the payload used by the target chunk's recipe.
                // If we find any, we should create deltas for them against the 
                // source.
                var dependencies = targetChunk.Recipes[0].GetDependencies();

                foreach (var targetPayload in dependencies)
                {
                    if (targetPayload.Type != ArchiveItemType.Payload)
                    {
                        continue;
                    }

                    string targetPayloadWildCardName = GetPayloadWildCardName(targetPayload.Name);
                    if (!sourcePayloadLookup.ContainsKey(targetPayloadWildCardName))
                    {
                        continue;
                    }

                    var sourcePayload = sourcePayloadLookup[targetPayloadWildCardName];

                    var key = $"{sourcePayload.Id}.{targetPayload.Id}";

                    bool targetIsCompressed = targetChunk.Recipes[0].IsCompressedDependency(targetPayload);
                    needed[key] = new Tuple<ArchiveItem, ArchiveItem, bool>(sourcePayload, targetPayload, targetIsCompressed);
                }
            }

            return needed;
        }

        protected void CreatePayloadDeltas(ConcurrentDictionary<ulong, ArchiveItem> diffChunkLookupByOffset)
        {
            if (!UsePayloadDevicePaths && !UseSourceFile)
            {
                Logger.LogInformation("CreatePayloadDeltas: UsePayloadDevicePaths and UseSourceFile are false - nothing to do.");
                return;
            }

            ConcurrentBag<ArchiveItem> deltaPayloadItems = new();

            var needed = GetDeltasNeeded(diffChunkLookupByOffset);

            int processedCount = 0;
            ParallelOptions po = new() { CancellationToken = CancellationToken };
            Parallel.ForEach(needed.Keys, po, deltaNeeded =>
            {
                int newProcessedCount = Interlocked.Increment(ref processedCount);
                if (newProcessedCount % 1000 == 0)
                {
                     Logger.LogInformation($"Processed: {newProcessedCount}/{needed.Count}. Deltas: {deltaPayloadItems.Count}");
                }

                var deltaNeededItems = needed[deltaNeeded];

                var sourcePayload = deltaNeededItems.Item1;
                var targetPayload = deltaNeededItems.Item2;
                bool targetIsCompressed = deltaNeededItems.Item3;

                if (!TryDelta(sourcePayload, targetPayload, targetIsCompressed, PayloadDeltaBuilders, out ArchiveItem deltaPayload))
                {
                    var sourcePath = DiffBuilder.GetSourcePayloadPath(WorkingFolder, sourcePayload);
                    var targetPath = DiffBuilder.GetTargetPayloadPath(WorkingFolder, targetPayload);
                    FileInfo sourceFileInfo = new(sourcePath);
                    FileInfo targetFileInfo = new(targetPath);
                    if (!IsSmallFileSize(targetFileInfo.Length) && !IsCompressedFileName(targetPayload.Name))
                    {
                        Logger.LogInformation("File was eligible for diff but none was created. Source: {0} ({1} bytes), Target: {2} ({3} bytes)", sourcePath, sourceFileInfo.Length, targetPath, targetFileInfo.Length);
                    }
                    return;
                }

                deltaPayloadItems.Add(deltaPayload);
            });

            foreach (var payload in deltaPayloadItems)
            {
                DeltaTokens.ArchiveItems.Add(payload);
            }
        }

        protected void CreateChunkDeltas(ConcurrentDictionary<ulong, ArchiveItem> diffChunkLookupByOffset)
        {
            if (!UseSourceFile)
            {
                Logger.LogInformation("CreateChunkDeltas: UseSourceFile is false - nothing to do.");
                return;
            }

            ConcurrentBag<ArchiveItem> deltaPayloadItems = new();

            Dictionary<ArchiveItem, int> chunkIndexLookup = new();

            if (SourceTokens.Chunks.Length == TargetTokens.Chunks.Length)
            {
                for (int i = 0; i < TargetTokens.Chunks.Length; i++)
                {
                    chunkIndexLookup[TargetTokens.Chunks[i]] = i;
                }
            }

            using var targetArchiveStream = File.OpenRead(TargetFile);
            using var sourceArchiveStream = File.OpenRead(SourceFile);

            int processedCount = 0;
            foreach (var targetChunk in TargetTokens.Chunks)
            {
                CheckForCancellation();
                int newProcessedCount = System.Threading.Interlocked.Increment(ref processedCount);
                if (newProcessedCount % 1000 == 0)
                {
                    Logger.LogInformation($"Processed: {newProcessedCount}/{TargetTokens.Chunks.Length}. Deltas: {deltaPayloadItems.Count}");
                }

                // First check if this item is useful for the Diff
                var diffChunk = diffChunkLookupByOffset[targetChunk.Offset.Value];

                // If we already derived a recipe here, then don't do anything else
                if (diffChunk.Recipes.Count != 0)
                {
                    continue;
                }

                if (chunkIndexLookup.ContainsKey(targetChunk))
                {
                    int index = chunkIndexLookup[targetChunk];

                    var sourceChunk = SourceTokens.Chunks[index];

                    var targetChunkPath = DiffBuilder.GetTargetChunkPath(WorkingFolder, targetChunk);
                    var sourceChunkPath = DiffBuilder.GetSourceChunkPath(WorkingFolder, sourceChunk);

                    ChunkExtractor.ExtractChunk(targetChunk, targetArchiveStream, targetChunkPath);
                    ChunkExtractor.ValidateExtractedChunk(targetChunk, targetChunkPath);
                    ChunkExtractor.ExtractChunk(sourceChunk, sourceArchiveStream, sourceChunkPath);
                    ChunkExtractor.ValidateExtractedChunk(sourceChunk, sourceChunkPath);

                    if (!TryDelta(sourceChunk, targetChunk, false, ChunkDeltaBuilders, out ArchiveItem deltaChunk))
                    {
                        continue;
                    }

                    DeltaTokens.ArchiveItems.Add(deltaChunk);
                }
            }
        }

        private ConcurrentDictionary<string, ArchiveItem> BuildArchiveItemLookupByName(IEnumerable<ArchiveItem> items)
        {
            ConcurrentDictionary<string, ArchiveItem> lookup = new();
            Parallel.ForEach(items, item =>
            {
                string wildCardName = GetPayloadWildCardName(item.Name);
                lookup[wildCardName] = item;
            });

            return lookup;
        }

        private string GetPayloadWildCardName(string fileName)
        {
            return Regex.Replace(fileName, @"\d", "*");
        }

        private ConcurrentDictionary<ulong, ArchiveItem> BuildArchiveItemLookupByOffset(IEnumerable<ArchiveItem> items)
        {
            ConcurrentDictionary<ulong, ArchiveItem> lookup = new();
            foreach (var item in items)
            {
                lookup[item.Offset.Value] = item;
            }

            return lookup;
        }

        List<DeltaBuilder> PayloadDeltaBuilders => new()
        {
            new NestedDeltaBuilder()
            {
                WorkingFolder = WorkingFolder,
                LogFolder = LogFolder,
                KeepWorkingFolder = KeepWorkingFolder,
                CancellationToken = CancellationToken
            },
            new BsDiffDeltaBuilder() { CancellationToken = CancellationToken },
            new ZstdDeltaBuilder() { CancellationToken = CancellationToken },
        };

        List<DeltaBuilder> ChunkDeltaBuilders => new()
        {
            new BsDiffDeltaBuilder() { CancellationToken = CancellationToken },
            new ZstdDeltaBuilder() { CancellationToken = CancellationToken },
        };

        bool TryDelta(ArchiveItem source, ArchiveItem target, bool targetIsCompressed, IEnumerable<DeltaBuilder> builders, out ArchiveItem deltaPayload)
        {
            string sourcePath = DiffBuilder.GetSourceArchiveItemPath(WorkingFolder, source);
            string targetPath = DiffBuilder.GetTargetArchiveItemPath(WorkingFolder, target);
            string baseDeltaPath = DiffBuilder.GetDeltaPath(WorkingFolder, source, target);

            foreach (var builder in builders)
            {
                CheckForCancellation();
                if (!builder.ShouldDelta(source, target))
                {
                    continue;
                }

                string deltaPath = builder.GetDecoratedPath(baseDeltaPath);
                if (!builder.Call(Logger, sourcePath, targetPath, targetIsCompressed, deltaPath))
                {
                    continue;
                }

                CheckForCancellation();
                FileInfo deltaFileInfo = new(deltaPath);
                if (deltaFileInfo.Length < 0)
                {
                    throw new Exception($"File length of delta was negative. Source: {sourcePath}, Target: {targetPath}, Delta: {deltaPath}");
                }

                // Delta is larger than original file.
                if ((ulong)deltaFileInfo.Length >= target.Length)
                {
                    deltaPayload = null;
                    continue;
                }

                ArchiveItemRecipeParameter sourceParam;
                if (source.Type == ArchiveItemType.Chunk)
                {
                    var sourceBlob = new ArchiveItem(source.Name, ArchiveItemType.Blob, null, source.Length, null, source.Hashes);
                    sourceBlob.Recipes.Add(new CopyRecipe(source));

                    sourceParam = new(sourceBlob);
                }
                else
                {
                    sourceParam = new (source);
                }

                ProcessHelper.WaitForFileAvailable(deltaPath);

                // TODO: We could simplify this a bit.
                var hashValue = Hash.Sha256File(deltaPath);
                CheckForCancellation();
                Dictionary<HashAlgorithmType, string> hashes = new();
                hashes[HashAlgorithmType.Sha256] = HexUtility.ByteArrayToHexString(hashValue);

                var deltaArchiveItem = new ArchiveItem(deltaPath, ArchiveItemType.Blob, null, (ulong)deltaFileInfo.Length, deltaPath, hashes);
                deltaArchiveItem.Recipes.Add(new InlineAssetRecipe());
                var deltaParam = new ArchiveItemRecipeParameter(deltaArchiveItem);

                var recipe = builder.MakeRecipe(deltaParam, sourceParam);
                deltaPayload = target.MakeCopy(false);
                deltaPayload.Recipes.Add(recipe);

                return true;
            }

            deltaPayload = null;
            return false;
        }
    }
}
