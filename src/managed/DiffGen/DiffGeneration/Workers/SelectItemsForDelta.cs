/**
 * @file SelectItemsForDelta.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class SelectItemsForDelta : Worker
{
    public string SourceFile { get; set; }

    public string TargetFile { get; set; }

    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public Diff Diff { get; set; }

    public HashSet<ItemDefinition> SourceItemsNeeded { get; set; } = new();

    public HashSet<ItemDefinition> TargetItemsNeeded { get; set; } = new();

    public DeltaPlans DeltaPlans { get; set; } = new();

    public SelectItemsForDelta(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    private void AddPayloadDeltas()
    {
        if (TargetTokens.Payload == null || !TargetTokens.Payload.Any() || SourceTokens.Payload == null || !SourceTokens.Payload.Any())
        {
            Logger.LogError("No payloads to process.");
            return;
        }

        List<string> newItems = new();
        List<string> identicalItems = new();
        List<string> differentItems = new();

        ulong totalNew = 0;
        ulong totalIdentical = 0;
        ulong totalDifferent = 0;

        foreach (var payloadEntry in TargetTokens.Payload)
        {
            var targetPayloadFullPath = payloadEntry.Key.Name;
            var targetPayloadItems = payloadEntry.Value;

            // This will also match exact matches
            var sourceWildcardMatches = SourceTokens.GetPayloadMatchingWildcard(targetPayloadFullPath);

            if (sourceWildcardMatches.Count() == 0)
            {
                newItems.Add($"{payloadEntry.Key.Name}, {string.Join(",", payloadEntry.Value.Select(x => x.ToString()))}");

                ulong totalItemBytes = 0;
                foreach (var item in targetPayloadItems)
                {
                    totalItemBytes += item.Length;
                }

                totalNew += totalItemBytes;

                continue;
            }

            foreach (var targetItem in targetPayloadItems)
            {
                foreach (var sourceItem in sourceWildcardMatches)
                {
                    if (targetItem.Equals(sourceItem))
                    {
                        identicalItems.Add($"{payloadEntry.Key.Name},{string.Join(",", payloadEntry.Value.Select(x => x.ToString()))}");
                        totalIdentical += targetItem.Length;
                        continue;
                    }

                    // If this item is an archive then it will be assembled from
                    // its recipes those items should be diffed, not the archive itself
                    if (TargetTokens.HasArchiveItem(targetItem))
                    {
                        break;
                    }

                    differentItems.Add($"{payloadEntry.Key.Name}, {string.Join(",", payloadEntry.Value.Select(x => x.ToString()))}");

                    totalDifferent += targetItem.Length;

                    if (Diff.IsNeededItem(targetItem))
                    {
                        var plan = new DeltaPlan(sourceItem, targetItem, targetPayloadFullPath, true);
                        DeltaPlans.AddDeltaPlan(targetItem, plan);
                        TargetItemsNeeded.Add(targetItem);
                        SourceItemsNeeded.Add(sourceItem);
                    }
                }
            }
        }

        var newItemsFile = Path.Combine(WorkingFolder, "NewItems.txt");
        File.WriteAllLines(newItemsFile, newItems);

        var identicalItemsFile = Path.Combine(WorkingFolder, "IdenticalItems.txt");
        File.WriteAllLines(identicalItemsFile, identicalItems);

        var differentItemsFile = Path.Combine(WorkingFolder, "DifferentItems.txt");
        File.WriteAllLines(differentItemsFile, differentItems);

        Logger.LogInformation("Total new items found with no basis: Count: {newItemsCount:N0}, Bytes: {totalNew:N0}. Details written to {newItemsFile}", newItems.Count(), totalNew, newItemsFile);
        Logger.LogInformation("Total identical items found: Count: {identicalItemsCount:N0}, Bytes: {totalIdentical:N0}. Details written to {identicalItemsFile}", identicalItems.Count(), totalIdentical, identicalItemsFile);
        Logger.LogInformation("Total different items found: Count: {differentItemsCount:N0}, Bytes: {totalDifferent:N0}. Details written to {differentItemsFile}", differentItems.Count(), totalDifferent, differentItemsFile);
    }

    private void AddSizeBasedDeltas()
    {
        Dictionary<ulong, HashSet<ItemDefinition>> sourceItemsBySize = new();

        // build index of items with recipes by size from source
        foreach (var (result, recipes) in SourceTokens.Recipes)
        {
            if (!sourceItemsBySize.ContainsKey(result.Length))
            {
                sourceItemsBySize.Add(result.Length, new());
            }

            sourceItemsBySize[result.Length].Add(result);
        }

        foreach (var needed in Diff.GetNeededItems())
        {
            if (needed.Length < 256)
            {
                continue;
            }

            if (DeltaPlans.HasPlan(needed))
            {
                continue;
            }

            if (sourceItemsBySize.ContainsKey(needed.Length))
            {
                bool matched = false;
                foreach (var sourceItem in sourceItemsBySize[needed.Length])
                {
                    if (AnyMatchingNames(needed, sourceItem))
                    {
                        matched = true;

                        var plan = new DeltaPlan(sourceItem, needed, string.Empty, false);
                        DeltaPlans.AddDeltaPlan(needed, plan);
                        TargetItemsNeeded.Add(needed);
                        SourceItemsNeeded.Add(sourceItem);
                        break;
                    }
                }

                if (matched)
                {
                    continue;
                }

                foreach (var sourceItem in sourceItemsBySize[needed.Length])
                {
                    var plan = new DeltaPlan(sourceItem, needed, string.Empty, false);
                    DeltaPlans.AddDeltaPlan(needed, plan);
                    TargetItemsNeeded.Add(needed);
                    SourceItemsNeeded.Add(sourceItem);
                }
            }
        }
    }

    private bool AnyMatchingNames(ItemDefinition left, ItemDefinition right)
    {
        return left.Names.Any(x => right.Names.Contains(x));
    }

    protected override void ExecuteInternal()
    {
        AddPayloadDeltas();
        AddSizeBasedDeltas();

        using (var stream = File.Create(Path.Combine(WorkingFolder, "DeltaPlans.json")))
        {
            DeltaPlans.WriteJson(stream, true);
        }

        Logger.LogInformation("DeltaPlans contains {DeltaPlanCount} entries.", DeltaPlans.Entries.Count);
    }
}
