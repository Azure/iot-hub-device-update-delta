/**
 * @file ExtractItemsForDelta.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System;
using System.Collections.Generic;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class ExtractItemsForDelta : Worker
{
    public string SourceFile { get; set; }

    public string TargetFile { get; set; }

    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public IEnumerable<ItemDefinition> SourceItemsNeeded { get; set; }

    public IEnumerable<ItemDefinition> TargetItemsNeeded { get; set; }

    public ExtractItemsForDelta(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        if (!SourceTokens.TryExtractItems(Logger, SourceFile, SourceItemsNeeded))
        {
            throw new Exception($"Couldn't extract items for delta from: {SourceFile}");
        }

        if (!TargetTokens.TryExtractItems(Logger, TargetFile, TargetItemsNeeded))
        {
            throw new Exception($"Couldn't extract items for delta from: {TargetFile}");
        }
    }
}
