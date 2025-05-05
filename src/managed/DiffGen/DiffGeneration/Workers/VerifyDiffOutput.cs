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

public class VerifyDiffOutput : Worker
{
    public string OutputFile { get; set; }

    public string SourceFile { get; set; }

    public Diff Diff { get; set; }

    public VerifyDiffOutput(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        CheckForCancellation();

        Logger.LogInformation("Verifying Diff Output at: {0}", OutputFile);

        using var session = new DiffApi.DiffaSession();

        session.AddArchive(OutputFile);

        try
        {
            VerifyArchiveItemProcessing(session);
            VerifyArchiveItemExtraction(session);
        }
        catch (Exception ex)
        {
            Logger.LogException(ex);

            VerifyInlineAssetProcessing(session);

            VerifyRemainderProcessing(session);
            throw;
        }
    }

    private void VerifyInlineAssetProcessing(DiffApi.DiffaSession session)
    {
        int successfulInlineAssetItems = 0;
        var inlineAssetRecipes = Diff.GetInlineAssetRecipes();
        Logger.LogInformation("Verifying {inlineAssetRecipesCount:N0} inline asset recipes.", inlineAssetRecipes.Count());
        foreach (var recipe in inlineAssetRecipes)
        {
            session.ClearRequestedItems();
            session.RequestItem(recipe.Result);
            if (!session.ProcessRequestedItems())
            {
                Logger.LogError("Couldn't process inline asset item: {recipeResult}. Offset: {recipeOffset:N0}", recipe.Result, recipe.NumberIngredients[0]);
                continue;
            }

            successfulInlineAssetItems++;
        }

        Logger.LogInformation("Successfully processed {successfulInlineAssetItems:N0} inline asset items.", successfulInlineAssetItems);
    }

    private void VerifyRemainderProcessing(DiffApi.DiffaSession session)
    {
        int successfulRemainderItems = 0;
        var remainderRecipes = Diff.GetRemainderRecipes();
        Logger.LogInformation("Verifying {remainderRecipesCount:N0} remainder recipes.", remainderRecipes.Count());
        foreach (var recipe in remainderRecipes)
        {
            session.ClearRequestedItems();
            session.RequestItem(recipe.Result);
            if (!session.ProcessRequestedItems())
            {
                Logger.LogError("Couldn't process remainder item: {recipeResult}. Offset: {recipeOffset:N0}", recipe.Result, recipe.NumberIngredients[0]);
                continue;
            }

            successfulRemainderItems++;
        }

        Logger.LogInformation("Successfully processed {successfulRemainderItems:N0} remainder items.", successfulRemainderItems);
    }

    private void VerifyArchiveItemProcessing(DiffApi.DiffaSession session)
    {
        session.AddItemToPantry(SourceFile);

        session.ClearRequestedItems();
        session.RequestItem(Diff.TargetItem);
        if (!session.ProcessRequestedItems())
        {
            Logger.LogError("Couldn't process archive item: {0}", Diff.TargetItem);
            throw new Exception("Couldn't verify diff. Archive Item failed to process.");
        }

        Logger.LogInformation("Successfully processed diff result item: {diffTokensArchiveItem}", Diff.TargetItem);
    }

    private void VerifyArchiveItemExtraction(DiffApi.DiffaSession session)
    {
        session.ResumeSlicing();

        var extractRoot = Path.Combine(WorkingFolder, "VerifyDiff");

        Directory.CreateDirectory(extractRoot);
        var resultExtractPath = Diff.TargetItem.GetExtractionPath(extractRoot);
        Logger.LogInformation("Extracting diff result item to: {resultExtractPath}", resultExtractPath);
        session.ExtractItemToPath(Diff.TargetItem, resultExtractPath);

        var testResult = ItemDefinition.FromFile(resultExtractPath);
        if (!testResult.Equals(Diff.TargetItem))
        {
            Logger.LogError("Extracted file at {resultExtractPath} didn't match expected Archive Item: {Diff.Tokens.ArchiveItem}", resultExtractPath, Diff.TargetItem);
            throw new Exception("Extracted file does not match expectded item.");
        }

        Logger.LogInformation("Extracted file at {resultExtractPath} matched Archive Item: {diffTokensArchiveItem}", resultExtractPath, Diff.TargetItem);

        session.CancelSlicing();
    }
}
