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

    private bool TryAddDeltaPlansFromSourceToTarget(HashSet<ItemDefinition> targetPayloadItems, string targetPayloadFullPath)
    {
        var sourcePayload = SourceTokens.GetPayloadWithName(targetPayloadFullPath);

        if (!sourcePayload.Any())
        {
            return false;
        }

        bool added = false;

        foreach (var targetItem in targetPayloadItems)
        {
            if (!Diff.IsNeededItem(targetItem))
            {
                continue;
            }

            // If this item is an archive then it will be assembled from
            // its recipes those items should be diffed, not the archive itself
            if (TargetTokens.HasArchiveItem(targetItem))
            {
                continue;
            }

            foreach (var sourceItem in sourcePayload)
            {
                if (!Diff.IsNeededItem(targetItem))
                {
                    continue;
                }

                var plan = new DeltaPlan(sourceItem, targetItem, targetPayloadFullPath, false);
                DeltaPlans.AddDeltaPlan(targetItem, plan);
                TargetItemsNeeded.Add(targetItem);
                SourceItemsNeeded.Add(sourceItem);

                added = true;
            }
        }

        return added;
    }

    protected override void ExecuteInternal()
    {
        AddPayloadDeltas();

        using (var stream = File.Create(Path.Combine(WorkingFolder, "DeltaPlans.json")))
        {
            DeltaPlans.WriteJson(stream, true);
        }

        Logger.LogInformation("DeltaPlans contains {DeltaPlanCount} entries.", DeltaPlans.Entries.Count);
    }
}
