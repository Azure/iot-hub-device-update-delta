/**
 * @file DeltaPlan.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs;

using ArchiveUtility;

public class DeltaPlan
{
    public ItemDefinition SourceItem { get; init; }

    public ItemDefinition TargetItem { get; init; }

    public string PayloadPath { get; init; }

    public bool IsWildcardMatch { get; init; }

    public DeltaPlan(ItemDefinition sourceItem, ItemDefinition targetItem, string payloadPath, bool isWildcardMatch)
    {
        SourceItem = sourceItem;
        TargetItem = targetItem;
        PayloadPath = payloadPath;
        IsWildcardMatch = isWildcardMatch;
    }
}
