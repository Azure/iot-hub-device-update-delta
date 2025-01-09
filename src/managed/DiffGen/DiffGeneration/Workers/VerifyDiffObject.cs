/**
 * @file VerifyDiff.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.x
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class VerifyDiffObject : Worker
{
    public Diff Diff { get; set; }

    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public VerifyDiffObject(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        CheckForCancellation();

        Logger.LogInformation("Verifying Diff Object.");

        List<ItemDefinition> items = new();

        items.Add(TargetTokens.ArchiveItem);
        int missingRecipes = 0;

        while (items.Count > 0)
        {
            List<ItemDefinition> nextItems = new();

            foreach (var toVerify in items)
            {
                if (toVerify.CompareTo(SourceTokens.ArchiveItem) == 0)
                {
                    continue;
                }

                if (!Diff.Tokens.ForwardRecipes.ContainsKey(toVerify))
                {
                    Logger.LogError("Couldn't find a recipe for: {toVerify}", toVerify);
                }
            }

            items = nextItems;
        }

        if (missingRecipes != 0)
        {
            Logger.LogError("Failed to find recipes for items: {missingRecipes}", missingRecipes);

            throw new DiffBuilderException($"Failed to find recipes for items: {missingRecipes}", FailureType.DiffGeneration);
        }
    }
}
