/**
* @file CreateDiff.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class CreateDiffRecipes : Worker
{
    public Diff Diff { get; set; }

    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public string TargetFile { get; set; }

    public DeltaCatalog DeltaCatalog { get; set; }

    public CreateDiffRecipes(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        // Copy the collection, because we will be modifying it
        var neededItems = Diff.GetNeededItems().ToList();
        var neededItemsCount = neededItems.Count();
        Logger.LogInformation("{needItemsCount} before processing DeltaCatalog.", neededItemsCount);

        foreach (var neededItem in neededItems)
        {
            // We successfully created a delta for this, we can use it
            if (DeltaCatalog.TryGetRecipe(neededItem, out Recipe recipe))
            {
                if (!recipe.IsDeltaRecipe())
                {
                    throw new Exception($"Found a non-delta recipe from delta catalog for item: {neededItem}, recipe: {recipe}");
                }

                var basisItem = recipe.GetDeltaBasis();
                Diff.AddRecipe(recipe);
            }
        }

        Diff.SelectRecipes();

        CreateInlineAssets();

        neededItems = Diff.GetNeededItems().ToList();
        var neededItemsCountAfterInlineAssets = neededItems.Count();
        Logger.LogInformation("{needItemsCount} after creating inline assets for delta.", neededItemsCountAfterInlineAssets);

        CreateRemainder(neededItems);
    }

    private void CreateInlineAssets()
    {
        HashSet<ItemDefinition> deltaItems = Diff.GetDeltaItems();

        Diff.InlineAssetsPath = GetInlineAssetsPath(WorkingFolder);

        var sortedDeltaItems = deltaItems.Order();

        using (var inlineAssetsFileWriter = File.Create(Diff.InlineAssetsPath))
        {
            foreach (var deltaItem in sortedDeltaItems)
            {
                var deltaFile = DeltaCatalog.GetDeltaFile(deltaItem);

                using var deltaFileStream = File.OpenRead(deltaFile);
                byte[] bytes = new byte[deltaItem.Length];
                deltaFileStream.Read(bytes, 0, bytes.Length);
                inlineAssetsFileWriter.Write(bytes);
            }
        }

        Diff.InlineAssetsItem = ItemDefinition.FromFile(Diff.InlineAssetsPath).WithName("inline_assets");
        Diff.RemoveNeeded(Diff.InlineAssetsItem);

        using (var stream = File.OpenRead(Diff.InlineAssetsPath))
        {
            using var reader = new BinaryReader(stream);

            ulong offset = 0;
            foreach (var item in sortedDeltaItems)
            {
                // We could count the entries, but that's slower
                // We only want to create a slice if this item isn't the entire thing
                if (item.Length != Diff.InlineAssetsItem.Length)
                {
                    Recipe slice = new("slice", item, new() { offset }, new() { Diff.InlineAssetsItem });
                    Diff.AddRecipe(slice);
                }

                stream.Seek((long)offset, SeekOrigin.Begin);

                var itemFromReader = ItemDefinition.FromBinaryReader(reader, item.Length);

                var deltaFile = DeltaCatalog.GetDeltaFile(item);

                offset += item.Length;

                if (!itemFromReader.Equals(item))
                {
                    throw new Exception($"InlineAssets item mismatch. Expected: {item}, Actual: {itemFromReader}");
                }
            }
        }
    }

    public static string GetInlineAssetsPath(string workingFolder) => Path.Combine(workingFolder, "InlineAssets.dat");

    private void CreateRemainder(IEnumerable<ItemDefinition> neededItems)
    {
        Logger.LogInformation("Adding remaining needed items to remainder.");

        var totalSize = neededItems.Sum(x => (long)x.Length);

        Logger.LogInformation("Found {count} dependencies for recipes with no recipes. Total size of remainder items: {totalSize:N0}", neededItems.Count(), totalSize);

        var sortedRemainderItems = neededItems.OrderBy(x => x.Length);

        if (!TargetTokens.TryExtractItems(Logger, TargetFile, neededItems))
        {
            Logger.LogError("Tried to extract and failed for files:");
            foreach (var item in neededItems)
            {
                Logger.LogError("\t{item}", item);
            }

            throw new Exception($"Couldn't extract remainder items from: {TargetFile}");
        }

        WriteRemainderFile(sortedRemainderItems);

        WriteRemainderItemsToJsonFile(sortedRemainderItems);

        CreateRemainderRecipes(sortedRemainderItems);

        VerifyRemainderItems(sortedRemainderItems);
    }

    private void WriteRemainderFile(IEnumerable<ItemDefinition> remainderItems)
    {
        Diff.RemainderPath = GetRemainderPath(WorkingFolder);
        int offsetInRemainderBlob = 0;

        using (var remainderStream = File.Create(Diff.RemainderPath))
        {
            foreach (var item in remainderItems)
            {
                var itemPath = item.GetExtractionPath(TargetTokens.ItemFolder);
                var itemStream = File.OpenRead(itemPath);

                if (itemStream.Length != (long)item.Length)
                {
                    throw new Exception($"Mismatch between extracted item path for item: {item}. Actual length for file {itemPath} is {itemStream.Length}");
                }

                int currentWriteOffset = (int)remainderStream.Length;

                if (currentWriteOffset != offsetInRemainderBlob)
                {
                    throw new Exception($"Mismatch between current offset in stream and expected current write offset based on items. Expected: {currentWriteOffset}, Actual: {offsetInRemainderBlob}!");
                }

                itemStream.CopyTo(remainderStream);

                offsetInRemainderBlob += (int)item.Length;
            }
        }

        using var stream = File.OpenRead(Diff.RemainderPath);
        using var reader = new BinaryReader(stream);
        Diff.RemainderItem = ItemDefinition.FromBinaryReader(reader, (ulong)stream.Length).WithName("remainder.uncompressed");
        Diff.RemoveNeeded(Diff.RemainderItem);
    }

    private void WriteRemainderItemsToJsonFile(IEnumerable<ItemDefinition> items)
    {
        var path = Path.Combine(WorkingFolder, "remainder.json");
        var json = ItemsToJson(items, true);

        File.WriteAllText(path, json);
    }

    private static string ItemsToJson(IEnumerable<ItemDefinition> items, bool indented)
    {
        var options = ArchiveTokenization.GetStandardJsonSerializerOptions();
        options.WriteIndented = indented;

        return JsonSerializer.Serialize(items, options);
    }

    private void CreateRemainderRecipes(IEnumerable<ItemDefinition> remainderItems)
    {
        ulong offsetInRemainderBlob = 0;
        List<ItemDefinition> items = new() { Diff.RemainderItem };

        if (remainderItems.Count() <= 1)
        {
            return;
        }

        foreach (var item in remainderItems)
        {
            List<ulong> numbers = new() { offsetInRemainderBlob };
            Recipe recipe = new("slice", item, numbers, items);

            Diff.AddRecipe(recipe);

            offsetInRemainderBlob += item.Length;
        }
    }

    private void VerifyRemainderItems(IEnumerable<ItemDefinition> items)
    {
        long offset = 0;
        using (var remainderStream = File.OpenRead(Diff.RemainderPath))
        {
            foreach (var item in items)
            {
                remainderStream.Seek(offset, SeekOrigin.Begin);

                using BinaryReader reader = new BinaryReader(remainderStream, Encoding.Default, true);
                var fromStream = ItemDefinition.FromBinaryReader(reader, item.Length);

                if (!item.Equals(fromStream))
                {
                    Logger.LogInformation("Failed to verify item at offset {offset}. Expected: {item}, Actual: {actual}", offset, item, fromStream);
                    throw new Exception($"Failed to verify item in remainder data.");
                }

                offset += (long)item.Length;
            }
        }
    }

    public static string GetRemainderPath(string workingFolder) => Path.Combine(workingFolder, "Remainder.dat");
}
