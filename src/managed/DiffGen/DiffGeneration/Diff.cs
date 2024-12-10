/**
* @file Diff.cs
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

public class Diff
{
    // For a given needed item, what parents depend on it?
    private Dictionary<ItemDefinition, HashSet<ItemDefinition>> _needItemParentMap = new();

    // For a given needed item, what dependencies does it have?
    private Dictionary<ItemDefinition, HashSet<ItemDefinition>> _neededItemDependencyMap = new();

    public ulong Version { get; set; } = 0;

    public ArchiveTokenization Tokens { get; set; } = new("Diff", "Standard");

    public string RemainderPath { get; set; }

    public string InlineAssetsPath { get; set; }

    public List<KeyValuePair<ItemDefinition, HashSet<ItemDefinition>>> NeededItems
    {
        get => _needItemParentMap.ToList();
        set
        {
            foreach (var entry in value)
            {
                var item = entry.Key;
                var parents = entry.Value;

                foreach (var parent in parents)
                {
                    SetParentNeedsItem(parent, item);
                }
            }
        }
    }

    public HashSet<ItemDefinition> SolvedItems { get; set; } = new();

    public Diff(ArchiveTokenization sourceTokens, ArchiveTokenization targetTokens)
    {
        Tokens.SourceItem = sourceTokens.ArchiveItem;
        Tokens.ArchiveItem = targetTokens.ArchiveItem;

        HashSet<ItemDefinition> needed = new();
        HashSet<ItemDefinition> solved = new();
        CheckSolutions(targetTokens, targetTokens.ArchiveItem, targetTokens.ArchiveItem);
    }

    private bool CheckSolutions(ArchiveTokenization tokens, ItemDefinition parent, ItemDefinition item)
    {
        bool solved = true;

        var itemKey = item.WithoutNames();
        if (tokens.ForwardRecipes.TryGetValue(itemKey, out var recipe))
        {
            foreach (var ingredient in recipe.ItemIngredients)
            {
                if (!CheckSolutions(tokens, itemKey, ingredient))
                {
                    solved = false;
                }
            }

            if (!solved)
            {
                SetParentNeedsItem(parent, itemKey);
            }
            else
            {
                Tokens.AddRecipe(recipe);
                SolvedItems.Add(itemKey);
            }

            return solved;
        }

        SetParentNeedsItem(parent, item);
        return false;
    }

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

    public IEnumerable<ItemDefinition> GetMissingDependencies()
    {
        var missing = new HashSet<ItemDefinition>();
        var missingList = new List<ItemDefinition>();

        foreach (var entry in Tokens.Recipes)
        {
            var recipes = entry.Value;

            foreach (var recipe in recipes)
            {
                var dependencies = recipe.ItemIngredients;

                foreach (var dep in dependencies)
                {
                    if (dep.Equals(Tokens.SourceItem))
                    {
                        continue;
                    }

                    if (!missing.Contains(dep) && !Tokens.HasAnyRecipes(dep))
                    {
                        missing.Add(dep);
                        missingList.Add(dep);
                    }
                }
            }
        }

        return missingList;
    }

    public IEnumerable<ItemDefinition> GetAllDependencies()
    {
        var dependencies = new HashSet<ItemDefinition>();
        var dependenciesList = new List<ItemDefinition>();

        foreach (var entry in Tokens.Recipes)
        {
            var recipes = entry.Value;

            foreach (var recipe in recipes)
            {
                foreach (var dep in recipe.ItemIngredients)
                {
                    if (!dependencies.Contains(dep))
                    {
                        dependencies.Add(dep);
                        dependenciesList.Add(dep);
                    }
                }
            }
        }

        return dependenciesList;
    }

    public bool IsNeeded(ItemDefinition item)
    {
        var itemKey = item.WithoutNames();

        return _needItemParentMap.ContainsKey(itemKey);
    }

    public bool IsSolved(ItemDefinition item)
    {
        var itemKey = item.WithoutNames();

        return SolvedItems.Contains(itemKey);
    }

    public void UnsetItemNeeded(ItemDefinition item)
    {
        var itemKey = item.WithoutNames();

        if (_neededItemDependencyMap.TryGetValue(itemKey, out var dependencies))
        {
            // remove our entry for dependencies
            _neededItemDependencyMap.Remove(itemKey);

            // if this item has any dependencies then remove this
            // from its parent list remove it if there are no other parents
            foreach (var dep in dependencies)
            {
                if (_needItemParentMap.TryGetValue(dep, out var parentList))
                {
                    if (parentList.Contains(item))
                    {
                        parentList.Remove(item);
                    }

                    if (parentList.Count() == 0)
                    {
                        UnsetItemNeeded(dep);
                    }
                }
            }
        }

        if (_needItemParentMap.TryGetValue(itemKey, out var parents))
        {
            foreach (var parent in parents)
            {
                if (_neededItemDependencyMap.TryGetValue(parent, out var deps))
                {
                    deps.Remove(itemKey);
                }
            }

            _needItemParentMap.Remove(itemKey);
        }
    }

    private void SetItemAsSolved(ItemDefinition item)
    {
        var itemKey = item.WithoutNames();

        UnsetItemNeeded(itemKey);

        SolvedItems.Add(itemKey);
    }

    public void AddRecipes(IEnumerable<Recipe> recipes, bool solved)
    {
        foreach (var recipe in recipes)
        {
            AddRecipe(recipe, solved);
        }
    }

    public void AddRecipe(Recipe recipe, bool solved)
    {
        var result = recipe.Result;

        Tokens.AddRecipe(recipe);

        if (solved)
        {
            SetItemAsSolved(result);
        }
    }

    public void SetParentNeedsItem(ItemDefinition parent, ItemDefinition item)
    {
        var parentKey = parent.WithoutNames();
        var itemKey = item.WithoutNames();

        if (!_needItemParentMap.ContainsKey(itemKey))
        {
            _needItemParentMap.Add(itemKey, new());
        }

        _needItemParentMap[itemKey].Add(parent);

        if (!_neededItemDependencyMap.ContainsKey(parentKey))
        {
            _neededItemDependencyMap.Add(parentKey, new());
        }

        _neededItemDependencyMap[parentKey].Add(itemKey);
    }

    public static bool TryGetReverseSolution(ArchiveTokenization tokens, ItemDefinition item, ref List<Recipe> recipes)
    {
        if (item.Equals(tokens.ArchiveItem))
        {
            return true;
        }

        var itemKey = item.WithoutNames();

        if (!tokens.ReverseRecipes.ContainsKey(itemKey))
        {
            return false;
        }

        bool solved = true;

        var recipe = tokens.ReverseRecipes[itemKey];

        recipes.Add(recipe);

        foreach (var ingredient in recipe.ItemIngredients)
        {
            if (!TryGetReverseSolution(tokens, ingredient, ref recipes))
            {
                solved = false;
                break;
            }
        }

        return solved;
    }
}