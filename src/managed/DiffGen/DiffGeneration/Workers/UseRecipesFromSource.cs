/**
 * @file UseRecipesFromSource.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.Collections.Generic;
using System.Linq;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class UseRecipesFromSource : Worker
{
    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public Diff Diff { get; set; }

    public UseRecipesFromSource(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        AddUsefulRecipesFromSource(SourceTokens, TargetTokens);
    }

    private void AddUsefulRecipesFromSource(ArchiveTokenization sourceTokens, ArchiveTokenization targetTokens)
    {
#pragma warning disable SA1010 // Opening square brackets should be spaced correctly
        List<ItemDefinition> items = [targetTokens.ArchiveItem];

        while (items.Any())
        {
            List<ItemDefinition> nextItems = new();

            foreach (var item in items)
            {
                if (!Diff.IsNeeded(item))
                {
                    continue;
                }

                List<Recipe> recipes = new();
                if (Diff.TryGetReverseSolution(sourceTokens, item, ref recipes))
                {
                    Logger.LogInformation("Found a reverse solution from source for item {item}", item);

                    Diff.AddRecipes(recipes, true);

                    continue;
                }
            }

            items = nextItems;
        }
    }
}
