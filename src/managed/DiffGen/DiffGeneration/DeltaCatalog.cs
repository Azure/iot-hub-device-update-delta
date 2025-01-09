/**
 * @file DeltaCatalog.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs;

using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;

using ArchiveUtility;

public class DeltaCatalog
{
    private Dictionary<ItemDefinition, Recipe> _recipes = new();

    public void ClearRecipes() => _recipes.Clear();

    public List<KeyValuePair<ItemDefinition, Recipe>> Recipes
    {
        get { return _recipes.ToList(); }
        set { _recipes = value.ToDictionary(x => x.Key, x => x.Value); }
    }

    public void AddRecipe(ItemDefinition item, Recipe recipe)
    {
        _recipes[item] = recipe;
    }

    public bool TryGetRecipe(ItemDefinition item, out Recipe recipes)
    {
        return _recipes.TryGetValue(item, out recipes);
    }

    private Dictionary<ItemDefinition, ItemDefinition> _targetItemToDeltaMap = new();

    public void ClearTargetItemToDeltaMap() => _targetItemToDeltaMap.Clear();

    public List<KeyValuePair<ItemDefinition, ItemDefinition>> TargetItemToDeltaMap
    {
        get { return _targetItemToDeltaMap.ToList(); }
        set { _targetItemToDeltaMap = value.ToDictionary(x => x.Key, x => x.Value); }
    }

    public void AddTargetItemDeltaEntry(ItemDefinition targetItem, ItemDefinition deltaItem)
    {
        _targetItemToDeltaMap.Add(targetItem, deltaItem);
    }

    private Dictionary<ItemDefinition, string> _deltaItemToDeltaFileMap = new();

    public void ClearDeltaItemToDeltaFileMap() => _deltaItemToDeltaFileMap.Clear();

    public List<KeyValuePair<ItemDefinition, string>> DeltaItemToDeltaFileMap
    {
        get { return _deltaItemToDeltaFileMap.ToList(); }
        set { _deltaItemToDeltaFileMap = value.ToDictionary(x => x.Key, x => x.Value); }
    }

    public void AddDeltaFile(ItemDefinition deltaItem, string deltaFile)
    {
        if (_deltaItemToDeltaFileMap.ContainsKey(deltaItem))
        {
            return;
        }

        _deltaItemToDeltaFileMap.Add(deltaItem, deltaFile);
    }

    public string GetDeltaFile(ItemDefinition deltaItem)
    {
        return _deltaItemToDeltaFileMap[deltaItem];
    }

    public IEnumerable<ItemDefinition> GetSortedDeltaItems()
    {
        var result = _deltaItemToDeltaFileMap.Select(x => x.Key).ToList();
        result.Sort();

        return result;
    }

    public static JsonSerializerOptions GetStandardJsonSerializerOptions()
    {
        var options = ArchiveTokenization.GetStandardJsonSerializerOptions();

        return options;
    }

    public static DeltaCatalog FromJsonPath(string path)
    {
        using var stream = File.OpenRead(path);
        return FromJson(stream);
    }

    public static DeltaCatalog FromJson(Stream stream)
    {
        using var reader = new StreamReader(stream);
        var jsonText = reader.ReadToEnd();
        return FromJson(jsonText);
    }

    public static DeltaCatalog FromJson(string jsonText)
    {
        var options = GetStandardJsonSerializerOptions();
        return JsonSerializer.Deserialize<DeltaCatalog>(jsonText, options);
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
