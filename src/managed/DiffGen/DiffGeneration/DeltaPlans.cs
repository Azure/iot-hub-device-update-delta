/**
 * @file FileList.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs;

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;

using ArchiveUtility;

public class DeltaPlans
{
    private Dictionary<ItemDefinition, HashSet<DeltaPlan>> _entries = new();

    public List<KeyValuePair<ItemDefinition, HashSet<DeltaPlan>>> Entries
    {
        get { return _entries.ToList(); }
        set { _entries = value.ToDictionary(x => x.Key, x => x.Value); }
    }

    public void AddDeltaPlan(ItemDefinition item, DeltaPlan deltaPlan)
    {
        var itemKey = item.WithoutNames();

        if (!_entries.ContainsKey(itemKey))
        {
            _entries.Add(itemKey, new());
        }

        _entries[itemKey].Add(deltaPlan);
    }

    public static JsonSerializerOptions GetStandardJsonSerializerOptions()
    {
        var options = ArchiveTokenization.GetStandardJsonSerializerOptions();

        return options;
    }

    public static DeltaPlans FromJsonPath(string path)
    {
        using var stream = File.OpenRead(path);
        return FromJson(stream);
    }

    public static DeltaPlans FromJson(Stream stream)
    {
        using var reader = new StreamReader(stream);
        var jsonText = reader.ReadToEnd();
        return FromJson(jsonText);
    }

    public static DeltaPlans FromJson(string jsonText)
    {
        if (string.IsNullOrEmpty(jsonText))
        {
            throw new ArgumentNullException(nameof(jsonText));
        }

        var options = GetStandardJsonSerializerOptions();
        return JsonSerializer.Deserialize<DeltaPlans>(jsonText, options);
    }

    public void WriteJson(Stream stream, bool writeIndented)
    {
        using var writer = new StreamWriter(stream, Encoding.UTF8, -1, true);
        var jsonText = ToJson(writeIndented);
        writer.Write(jsonText);
    }

    public string ToJson(bool writeIndented)
    {
        var options = GetStandardJsonSerializerOptions();
        options.WriteIndented = writeIndented;

        return JsonSerializer.Serialize(this, options);
    }

    public string ToJson()
    {
        return ToJson(false);
    }
}