/**
* @file AddRemainderChunks.cs
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

public class AddRemainderChunks : Worker
{
    public Diff Diff { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public string TargetFile { get; set; }

    public AddRemainderChunks(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        Logger.LogInformation("Determining remaining chunks.");

        var remainderItems = Diff.GetRemainderItems();

        var totalSize = remainderItems.Sum(x => (long)x.Length);

        var sortedRemainderItems = remainderItems.OrderByDescending(x => x.Length).ToList();

        Logger.LogInformation("Found {count} dependencies for recipes with no recipes. Total size of remainder items: {totalSize:N0}", remainderItems.Count(), totalSize);

        if (!TargetTokens.TryExtractItems(Logger, TargetFile, remainderItems))
        {
            throw new Exception($"Coulnd't extract remainder items for: {TargetFile}");
        }

        WriteRemainderFile(remainderItems);

        WriteRemainderItemsToJsonFile(remainderItems);

        CreateRemainderRecipes(remainderItems);

        VerifyRemainderItems(remainderItems);
    }

    private void WriteRemainderFile(IEnumerable<ItemDefinition> remainderItems)
    {
        Diff.RemainderPath = Path.Combine(WorkingFolder, "remainder.dat");
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
        Diff.Tokens.RemainderItem = ItemDefinition.FromBinaryReader(reader, (ulong)stream.Length).WithName("remainder.uncompressed");
    }

    private void CreateRemainderRecipes(IEnumerable<ItemDefinition> remainderItems)
    {
        ulong offsetInRemainderBlob = 0;
        List<ItemDefinition> items = new() { Diff.Tokens.RemainderItem };

        if (remainderItems.Count() <= 1)
        {
            return;
        }

        foreach (var item in remainderItems)
        {
            List<ulong> numbers = new() { offsetInRemainderBlob };
            Recipe recipe = new("slice", item, numbers, items);

            Diff.Tokens.AddRecipe(recipe);

            offsetInRemainderBlob += item.Length;
        }
    }

    private void WriteRemainderItemsToJsonFile(IEnumerable<ItemDefinition> items)
    {
        var path = Path.Combine(WorkingFolder, "remainder.json");
        var json = ToJson(items, true);

        File.WriteAllText(path, json);
    }

    private static string ToJson(IEnumerable<ItemDefinition> items, bool indented)
    {
        var options = ArchiveTokenization.GetStandardJsonSerializerOptions();
        options.WriteIndented = indented;

        return JsonSerializer.Serialize(items, options);
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
}