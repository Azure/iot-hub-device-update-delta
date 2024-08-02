/**
 * @file VerifyDiff.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System;
using System.IO;
using System.Linq;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class VerifyDiff : Worker
{
    public string OutputFile { get; set; }

    public string SourceFile { get; set; }

    public Diff Diff { get; set; }

    public VerifyDiff(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        CheckForCancellation();

        Logger.LogInformation("Verifying diff at: {0}", OutputFile);

        using var session = new DiffApi.DiffaSession();

        session.AddArchive(OutputFile);

        int successfulInlineAssetItems = 0;
        var inlineAssetRecipes = Diff.Tokens.GetInlineAssetRecipes();
        Logger.LogInformation("Verifying {0} inline asset recipes.", inlineAssetRecipes.Count());
        foreach (var recipe in inlineAssetRecipes)
        {
            session.ClearRequestedItems();
            session.RequestItem(recipe.Result);
            if (!session.ProcessRequestedItems())
            {
                Logger.LogError("Couldn't process inline asset item: {0}. Offset: {1}", recipe.Result, recipe.NumberIngredients[0]);
                continue;
            }

            successfulInlineAssetItems++;
        }

        Logger.LogInformation("Successfully processed {0} inline asset items.", successfulInlineAssetItems);

        int successfulRemainderItems = 0;
        var remainderRecipes = Diff.Tokens.GetRemainderRecipes();
        Logger.LogInformation("Verifying {0} remainder recipes.", remainderRecipes.Count());
        foreach (var recipe in remainderRecipes)
        {
            session.ClearRequestedItems();
            session.RequestItem(recipe.Result);
            if (!session.ProcessRequestedItems())
            {
                Logger.LogError("Couldn't process remainder item: {0}. Offset: {1}", recipe.Result, recipe.NumberIngredients[0]);
                continue;
            }

            successfulRemainderItems++;
        }

        Logger.LogInformation("Successfully processed {0} remainder items.", successfulRemainderItems);

        session.AddItemToPantry(SourceFile);

        var extractRoot = Path.Combine(WorkingFolder, "VerifyDiff");
        Directory.CreateDirectory(extractRoot);

        session.ClearRequestedItems();
        session.RequestItem(Diff.Tokens.ArchiveItem);
        if (!session.ProcessRequestedItems())
        {
            Logger.LogError("Couldn't process archive item: {0}", Diff.Tokens.ArchiveItem);
            throw new Exception("Couldn't verify diff. Archive Item failed to process.");
        }

        Logger.LogInformation("Successfully processed diff result item: {0}", Diff.Tokens.ArchiveItem);

        session.ResumeSlicing();

        var resultExtractPath = Diff.Tokens.ArchiveItem.GetExtractionPath(extractRoot);
        Logger.LogInformation("Extracting diff result item to: {0}", resultExtractPath);
        session.ExtractItemToPath(Diff.Tokens.ArchiveItem, resultExtractPath);

        var testResult = ItemDefinition.FromFile(resultExtractPath);
        if (!testResult.Equals(Diff.Tokens.ArchiveItem))
        {
            Logger.LogError("Extracted file at {0} didn't match expected Archive Item: {1}", resultExtractPath, Diff.Tokens.ArchiveItem);
            throw new Exception("Extracted file does not match expectded item.");
        }

        Logger.LogInformation("Extracted file at {0} matched Archive Item: {1}", resultExtractPath, Diff.Tokens.ArchiveItem);

        session.CancelSlicing();
    }
}
