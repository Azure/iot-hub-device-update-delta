/**
* @file CreateInlineAssets.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class CreateInlineAssets : Worker
{
    public Diff Diff { get; set; }

    public DeltaCatalog DeltaCatalog { get; set; }

    private HashSet<Recipe> _inlineAssetsRecipes = new();

    public CreateInlineAssets(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        Diff.InlineAssetsPath = GetInlineAssetsPath(WorkingFolder);

        var items = Diff.GetSortedDeltaItems();

        using (var inlineAssetsFileWriter = File.Create(Diff.InlineAssetsPath))
        {
            foreach (var deltaItem in items)
            {
                var deltaFile = DeltaCatalog.GetDeltaFile(deltaItem);

                using var deltaFileStream = File.OpenRead(deltaFile);
                byte[] bytes = new byte[deltaItem.Length];
                deltaFileStream.Read(bytes, 0, bytes.Length);
                inlineAssetsFileWriter.Write(bytes);
            }
        }

        Diff.Tokens.InlineAssetsItem = ItemDefinition.FromFile(Diff.InlineAssetsPath).WithName("inline_assets");

        using (var stream = File.OpenRead(Diff.InlineAssetsPath))
        {
            using var reader = new BinaryReader(stream);

            ulong offset = 0;
            foreach (var item in items)
            {
                // We could count the entries, but that's slower
                // We only want to create a slice if this item isn't the entire thing
                if (item.Length != Diff.Tokens.InlineAssetsItem.Length)
                {
                    Recipe slice = new("slice", item, new() { offset }, new() { Diff.Tokens.InlineAssetsItem });
                    Diff.Tokens.AddRecipe(slice);
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

    public static string GetInlineAssetsPath(string workingFolder)
    {
        return Path.Combine(workingFolder, "InlineAssets.dat");
    }
}
