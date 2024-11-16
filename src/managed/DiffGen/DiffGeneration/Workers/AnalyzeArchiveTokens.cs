namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

[SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
internal class AnalyzeArchiveTokens : Worker
{
    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public UInt64 TotalUndiffable { get; set; }

    public UInt64 UndiffableThreshold { get; set; }

    public bool IsDiffable { get; set; } = true;

    public AnalyzeArchiveTokens(ILogger logger, string workingFolder, CancellationToken cancellationToken)
     : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        Logger.LogInformation("Analyzing archive tokens");

        List<ItemDefinition> undiffableArchives = new();
        UInt64 totalUndiffable = 0;

        foreach (var archive in TargetTokens.ArchiveItems)
        {
            var archiveKey = archive.WithoutNames();

            // If we can produce this archive with some recipe, we may be able to make
            // a reasonable diff
            if (TargetTokens.ForwardRecipes.ContainsKey(archiveKey))
            {
                continue;
            }

            // If we can produce this archive from something in the source, we
            // may be able to have a reasonable diff
            if (SourceTokens.ReverseRecipes.ContainsKey(archiveKey))
            {
                continue;
            }

            // Otherwise, we don't have a recipe to produce this, it will be carried as a remainder item
            undiffableArchives.Add(archive);

            totalUndiffable += archive.Length;
        }

        if (totalUndiffable > UndiffableThreshold)
        {
            foreach (var archive in undiffableArchives)
            {
                Logger.LogInformation("Archive is not diffable: {archive}", archive.ToString());
            }

            Logger.LogError("Archive is not diffable");
            IsDiffable = false;
            return;
        }
    }
}
