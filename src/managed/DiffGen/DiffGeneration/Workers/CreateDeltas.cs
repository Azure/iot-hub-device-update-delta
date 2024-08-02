/**
* @file CreateDeltas.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

using ArchiveUtility;

using Microsoft.Azure.DeviceUpdate.Diffs;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

/// <summary>
///  This worker generates deltas that will be used for recipes to regenerate chunks in a diff.
///  We skip any chunks that have a recipe. We look at the diff and look for any dependencies
///  that are similar to entries in the source.
///  When we find one we use some simple measures to determine if we should create the diff (size)
///  and then call into bsdiff to create it.
///  The worker will produce an ArchiveTokenization named DeltaTokens with a set of payload
///  entries to define the recipes describing the diffs which will be used in subsequent steps.
/// </summary>
public class CreateDeltas : Worker
{
    public string SourceFile { get; set; }

    public string TargetFile { get; set; }

    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public Diff Diff { get; set; }

    public DeltaCatalog DeltaCatalog { get; set; } = new();

    public string DeltaCatalogPath { get; set; }

    public string LogFolder { get; set; }

    public bool UseSourceFile { get; set; }

    public bool KeepWorkingFolder { get; set; }

    public Dictionary<ItemDefinition, HashSet<DeltaPlan>> DeltaPlans { get; set; }

    public CreateDeltas(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    private static string GetNativePath(string path)
    {
        if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return path;
        }

        var nativePath = Path.GetFullPath(path);

        if (nativePath[1] != ':')
        {
            return nativePath;
        }

        nativePath = @"\\?\" + nativePath;

        return nativePath;
    }

    protected override void ExecuteInternal()
    {
        var cookiePath = Path.Combine(WorkingFolder, "deltas.cookie");
        var deltaCatalogPath = DiffBuilder.GetDeltaCatalogPath(WorkingFolder);

        int processedCount = 0;

        ConcurrentBag<List<Recipe>> deltaRecipeSets = new();
        ConcurrentDictionary<ItemDefinition, ItemDefinition> targetItemToDeltaItemMap = new();
        ConcurrentDictionary<ItemDefinition, string> deltaFiles = new();

        var sourceItemsRoot = SourceTokens.ItemFolder;
        var targetItemsRoot = TargetTokens.ItemFolder;
        var deltaRoot = DiffBuilder.GetDeltaRoot(WorkingFolder);

        Logger.LogInformation("Delta Plan has {Count} entries.", DeltaPlans.Count);

        ParallelOptions po = new() { CancellationToken = CancellationToken };
        Parallel.ForEach(DeltaPlans.Keys, po, itemToDelta =>
        {
            int newProcessedCount = Interlocked.Increment(ref processedCount);
            if (newProcessedCount % 1000 == 0)
            {
                Logger.LogInformation($"Processed: {newProcessedCount}/{DeltaPlans.Count}. Delta: {deltaRecipeSets.Count}");
            }

            var deltaPlansForItem = DeltaPlans[itemToDelta];

            foreach (var entry in deltaPlansForItem)
            {
                var sourceItem = entry.SourceItem;
                var targetItem = entry.TargetItem;

                var sourceFileHash = sourceItem.GetSha256HashString();
                var targetFilehash = targetItem.GetSha256HashString();

                // Don't try to create deltas between identical items
                if (string.Compare(targetFilehash, sourceFileHash) == 0)
                {
                    return;
                }

                var sourceFileName = Path.Combine(sourceItemsRoot, sourceFileHash);
                var targetFileName = Path.Combine(targetItemsRoot, targetFilehash);

                var baseDeltaFileName = $"{sourceItem.GetSha256HashString()}_{targetItem.GetSha256HashString()}";
                var baseDeltaFilePath = GetNativePath(Path.Combine(deltaRoot, baseDeltaFileName));

                if (TryDelta(
                    sourceItem,
                    sourceFileName,
                    targetItem,
                    targetFileName,
                    baseDeltaFilePath,
                    out ItemDefinition deltaItem,
                    out string deltaFilePath,
                    out List<Recipe> recipes))
                {
                    deltaRecipeSets.Add(recipes);
                    targetItemToDeltaItemMap.TryAdd(targetItem, deltaItem);
                    deltaFiles.TryAdd(deltaItem, deltaFilePath);

                    return;
                }
                else
                {
                    if (targetItem.Length + sourceItem.Length > 1000)
                    {
                        Logger.LogInformation("Couldn't generate delta for {0} and {1}", sourceFileName, targetFileName);
                    }
                }
            }
        });

        var totalDeltaSize = targetItemToDeltaItemMap.Select(x => (long)x.Value.Length).Sum();
        var totalTargetSize = targetItemToDeltaItemMap.Select(x => (long)x.Key.Length).Sum();

        Logger.LogInformation("Generated {0} deltas with total size {1} for {2} bytes in target.", targetItemToDeltaItemMap.Count(), totalDeltaSize, totalTargetSize);

        CheckForCancellation();

        foreach (var recipes in deltaRecipeSets)
        {
            foreach (var recipe in recipes)
            {
                DeltaCatalog.AddRecipe(recipe.Result, recipe);
            }
        }

        foreach (var entry in targetItemToDeltaItemMap)
        {
            var targetItem = entry.Key;
            var deltaItem = entry.Value;

            DeltaCatalog.AddTargetItemDeltaEntry(targetItem, deltaItem);

            var deltaFile = deltaFiles[deltaItem];

            DeltaCatalog.AddDeltaFile(deltaItem, deltaFile);
        }

        Logger.LogInformation("Writing DeltaCatalog to: {0}", deltaCatalogPath);
        using (var stream = File.Create(deltaCatalogPath))
        {
            DeltaCatalog.WriteJson(stream, true);
        }

        DeltaCatalogPath = deltaCatalogPath;
        CreateCookie(cookiePath);
    }

    private List<DeltaBuilder> builders => new()
    {
        new BsDiffDeltaBuilder(Logger) { CancellationToken = CancellationToken },
        new ZstdDeltaBuilder(Logger) { CancellationToken = CancellationToken },
    };

    private bool TryDelta(
        ItemDefinition sourceItem,
        string sourceFile,
        ItemDefinition targetItem,
        string targetFile,
        string baseDeltaFile,
        out ItemDefinition deltaItem,
        out string deltaFile,
        out List<Recipe> recipes)
    {
        List<Recipe> bestRecipes = null;
        string bestDeltaFile = null;
        ItemDefinition bestDeltaItem = targetItem;

        if (IsSmallFileSize((long)targetItem.Length))
        {
            deltaItem = null;
            deltaFile = null;
            recipes = null;
            return false;
        }

        foreach (var builder in builders)
        {
            CheckForCancellation();

            if (!builder.ShouldDelta(sourceItem, targetItem))
            {
                continue;
            }

            if (!builder.Call(
                Logger,
                sourceItem,
                sourceFile,
                targetItem,
                targetFile,
                baseDeltaFile,
                out ItemDefinition newDeltaItem,
                out string newDeltaFile,
                out List<Recipe> newRecipes))
            {
                continue;
            }

            // Builder indicated that it can generate the original file
            // from itself or otherwise had an error
            if (newDeltaItem.Length == 0)
            {
                continue;
            }

            if (newDeltaItem.Length >= bestDeltaItem.Length)
            {
                continue;
            }

            bestDeltaFile = newDeltaFile;
            bestDeltaItem = newDeltaItem;
            bestRecipes = newRecipes;
        }

        recipes = bestRecipes;
        deltaFile = bestDeltaFile;
        deltaItem = bestDeltaItem.Length < targetItem.Length ? bestDeltaItem : null;
        return deltaItem != null;
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

    private List<DeltaBuilder> ChunkDeltaBuilders => new()
    {
        new BsDiffDeltaBuilder(Logger) { CancellationToken = CancellationToken },
        new ZstdDeltaBuilder(Logger) { CancellationToken = CancellationToken },
    };
}
