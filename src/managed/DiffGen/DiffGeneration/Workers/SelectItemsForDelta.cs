/**
 * @file SelectItemsForDelta.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.Collections.Generic;
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

    public Dictionary<ItemDefinition, HashSet<DeltaPlan>> DeltaPlans { get; set; } = new();

    public SelectItemsForDelta(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected void AddPayloadDeltas()
    {
        foreach (var payloadEntry in TargetTokens.Payload)
        {
            var targetPayloadFullPath = payloadEntry.Key.Name;
            var targetPayloadItems = payloadEntry.Value;

            if (SourceTokens.HasPayloadWithName(targetPayloadFullPath))
            {
                var sourcePayload = SourceTokens.GetPayloadWithName(targetPayloadFullPath);

                foreach (var targetItem in targetPayloadItems)
                {
                    if (!Diff.IsNeeded(targetItem))
                    {
                        continue;
                    }

                    foreach (var sourceItem in sourcePayload)
                    {
                        if (targetItem.Equals(sourceItem))
                        {
                            List<Recipe> recipes = new();
                            if (Diff.TryGetReverseSolution(SourceTokens, sourceItem, ref recipes))
                            {
                                Diff.AddRecipes(recipes, true);
                            }

                            break;
                        }

                        // If this item is an archive then it will be assembled from
                        // its recipes those items should be diffed, not the archive itself
                        if (TargetTokens.HasArchiveItem(targetItem))
                        {
                            break;
                        }

                        var sourceItemKey = sourceItem.WithoutNames();
                        var targetItemKey = targetItem.WithoutNames();

                        if (!DeltaPlans.ContainsKey(targetItemKey))
                        {
                            DeltaPlans.Add(targetItemKey, new());
                        }

                        DeltaPlans[targetItemKey].Add(new(sourceItemKey, targetItemKey));
                        TargetItemsNeeded.Add(targetItemKey);
                        SourceItemsNeeded.Add(sourceItemKey);
                    }
                }
            }

            var sourceWildcardMatches = SourceTokens.GetPayloadMatchingWildcard(targetPayloadFullPath);

            foreach (var targetItem in targetPayloadItems)
            {
                foreach (var sourceItem in sourceWildcardMatches)
                {
                    if (targetItem.Equals(sourceItem))
                    {
                        continue;
                    }

                    var targetItemKey = targetItem.WithoutNames();
                    var sourceItemKey = sourceItem.WithoutNames();

                    // If this item is an archive then it will be assembled from
                    // its recipes those items should be diffed, not the archive itself
                    if (TargetTokens.HasArchiveItem(targetItem))
                    {
                        break;
                    }

                    if (!DeltaPlans.ContainsKey(targetItemKey))
                    {
                        DeltaPlans.Add(targetItemKey, new());
                    }

                    DeltaPlans[targetItemKey].Add(new(sourceItemKey, targetItemKey));
                    TargetItemsNeeded.Add(targetItemKey.WithoutNames());
                    SourceItemsNeeded.Add(sourceItemKey.WithoutNames());
                }
            }
        }
    }

    protected override void ExecuteInternal()
    {
        AddPayloadDeltas();
        Logger.LogInformation("DeltaPlans contains {DeltaPlanCount} entries.", DeltaPlans.Count);
    }
}
