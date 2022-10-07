/**
 * @file Diff.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.IO;
    using System.Collections.Generic;
    using System.Security.Authentication;
    using System.Security.Cryptography;
    using System.Text;
    using System.Text.Json;
    using System.Linq;


    using ArchiveUtility;

    class Diff
    {
        public ulong Version { get; set; } = 0;
        public ulong TargetSize { get; set; } = 0;
        public Hash TargetHash { get; set; }
        public ulong SourceSize { get; set; }
        public Hash SourceHash { get; set; }
        public List<ArchiveItem> Chunks { get; set; } = new();
        public string RemainderPath { get; set; }
        public ulong RemainderUncompressedSize { get; set; }
        public ulong RemainderCompressedSize { get; set; }

        public void WriteJson(Stream stream, bool writeIndented)
        {
            using (var writer = new StreamWriter(stream, Encoding.UTF8, -1, true))
            {
                var value = ToJson(writeIndented);

                writer.Write(value);
            }
        }

        public string ToJson(bool writeIndented)
        {
            var options = ArchiveTokenization.GetStandardJsonSerializerOptions();
            options.WriteIndented = writeIndented;

            return JsonSerializer.Serialize(this, options);
        }

        public IEnumerable<ArchiveItem> GetAllInlineAssetArchiveItems()
        {
            return Chunks.SelectMany(c =>
            {
                var dependencies = c.Recipes[0].GetDependencies();

                return dependencies.Where(d =>
                {
                    return d.Recipes.Count > 0 && d.Recipes[0].Type == RecipeType.InlineAsset;
                });
            });
        }

        public ulong GetTotalInlineAssetBytes()
        {
            return (ulong) GetAllInlineAssetArchiveItems().Sum(item => (long)item.Length);
        }
    }
}