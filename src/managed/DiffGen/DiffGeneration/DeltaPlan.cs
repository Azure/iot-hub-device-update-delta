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

    public DeltaPlan(ItemDefinition sourceItem, ItemDefinition targetItem)
    {
        SourceItem = sourceItem;
        TargetItem = targetItem;
    }
}
