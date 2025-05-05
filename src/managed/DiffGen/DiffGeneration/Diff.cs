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
using Microsoft.Extensions.Logging;

public class Diff
{
    public ulong Version { get; set; } = 0;

    private ArchiveTokenization _tokens { get; set; } = new("Diff", "Standard");

    public string RemainderPath { get; set; }

    public string InlineAssetsPath { get; set; }

    public ItemDefinition TargetItem
    {
        get
        {
            return _targetTokens.ArchiveItem;
        }

        set
        {
            _tokens.ArchiveItem = value;
            _targetTokens.ArchiveItem = value;
        }
    }

    public ItemDefinition SourceItem
    {
        get
        {
            return _sourceTokens.ArchiveItem;
        }

        set
        {
            _sourceTokens.ArchiveItem = value;
        }
    }

    public ItemDefinition InlineAssetsItem
    {
        get
        {
            return _tokens.InlineAssetsItem;
        }

        set
        {
            _tokens.InlineAssetsItem = value;
        }
    }

    public ItemDefinition RemainderItem
    {
        get
        {
            return _tokens.RemainderItem;
        }

        set
        {
            _tokens.RemainderItem = value;
        }
    }

    public IEnumerable<Recipe> Recipes { get => _tokens.Recipes; }

    public List<Recipe> DeltaRecipes { get; set; } = new();

    private ILogger _logger;
    private ArchiveTokenization _sourceTokens;
    private ArchiveTokenization _targetTokens;
    private string _workingFolder;

    private HashSet<ItemDefinition> _neededItems = new();

    private enum SearchResult
    {
        Success,
        Fail,
        Maybe,
    }

    public Diff(ILogger logger, ArchiveTokenization sourceTokens, ArchiveTokenization targetTokens, string workingFolder)
    {
        _tokens.SourceItem = sourceTokens.ArchiveItem;
        _tokens.ArchiveItem = targetTokens.ArchiveItem;

        _sourceTokens = sourceTokens;
        _targetTokens = targetTokens;

        _logger = logger;
        _workingFolder = workingFolder;

        HashSet<ItemDefinition> neededItems = new();

        // Consider all payload both missing and covered
        foreach (var payload in _targetTokens.Payload)
        {
            foreach (var item in payload.Value)
            {
                if (!_sourceTokens.HasAnyRecipes(item))
                {
                    _neededItems.Add(item);
                }
            }
        }
    }

    public void SelectRecipes()
    {
        HashSet<ItemDefinition> coveredItems = [_sourceTokens.ArchiveItem];

        var allRecipesSorted = _sourceTokens.GetAllRecipes().OrderBy(r => r.ItemIngredients.Count).ToList();

        bool changed;
        do
        {
            changed = false;
            foreach (var recipe in allRecipesSorted)
            {
                var result = recipe.Result;
                if (coveredItems.Contains(result))
                {
                    continue;
                }

                bool isValid = recipe.ItemIngredients.All(ingredient => coveredItems.Contains(ingredient));

                if (isValid)
                {
                    coveredItems.Add(result);
                    changed = true;
                }
            }
        }
        while (changed);

        foreach (var entry in _sourceTokens.ReverseRecipes)
        {
            var result = entry.Key;

            if (HasAnyRecipes(result))
            {
                continue;
            }

            var recipe = entry.Value;
            AddRecipe(recipe);
        }

        foreach (var entry in _targetTokens.ForwardRecipes)
        {
            var result = entry.Key;

            if (HasAnyRecipes(result))
            {
                continue;
            }

            var recipe = entry.Value;
            AddRecipe(recipe);
        }

        Queue<ItemDefinition> itemQueue = new();
        itemQueue.Enqueue(_targetTokens.ArchiveItem);
        HashSet<ItemDefinition> needed = new();
        HashSet<ItemDefinition> covered = new();

        _neededItems.Clear();

        while (itemQueue.Any())
        {
            var item = itemQueue.Dequeue();

            if (covered.Contains(item))
            {
                continue;
            }

            if (HasAnyRecipes(item))
            {
                var recipes = GetRecipes(item);

                if (recipes.Count != 1)
                {
                    throw new Exception($"Expected just one recipe for item: {item}");
                }

                var recipe = recipes.First();

                if (recipe.IsDeltaRecipe())
                {
                    itemQueue.Enqueue(recipe.GetDeltaBasis());
                }
                else
                {
                    foreach (var ingredient in recipes.First().ItemIngredients)
                    {
                        itemQueue.Enqueue(ingredient);
                    }
                }

                covered.Add(item);
                continue;
            }

            if (_sourceTokens.ForwardRecipes.TryGetValue(item, out var sourceRecipe))
            {
                AddRecipe(sourceRecipe);

                // This should be already satisfied
                if (sourceRecipe.ItemIngredients.Any(i => !HasAnyRecipes(i)))
                {
                    throw new Exception($"Found a recipe in source item, but the ingredients are not available already: {item}");
                }

                covered.Add(item);
                continue;
            }

            if (_targetTokens.ForwardRecipes.TryGetValue(item, out var targetRecipe))
            {
                AddRecipe(targetRecipe);

                foreach (var ingredient in targetRecipe.ItemIngredients)
                {
                    itemQueue.Enqueue(ingredient);
                }

                covered.Add(item);
                continue;
            }

            _neededItems.Add(item);
            covered.Add(item);
        }
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

    public IEnumerable<ItemDefinition> GetNeededItems() => _neededItems;

    public void AddRecipe(Recipe recipe)
    {
        _tokens.AddRecipe(recipe);
    }

    public bool HasAnyRecipes(ItemDefinition item) => _tokens.HasAnyRecipes(item);

    public HashSet<Recipe> GetRecipes(ItemDefinition item) => _tokens.GetRecipes(item);

    public HashSet<ItemDefinition> GetDeltaItems() => _tokens.GetAllRecipes().Where(r => r.IsDeltaRecipe()).Select(r => r.GetDeltaItem()).ToHashSet();

    public IEnumerable<Recipe> GetRemainderRecipes() => _tokens.GetRemainderRecipes();

    public IEnumerable<Recipe> GetInlineAssetRecipes() => _tokens.GetInlineAssetRecipes();

    public void ClearRecipes() => _tokens.ClearRecipes();

    public bool IsNeeded(ItemDefinition item) => _neededItems.Contains(item);

    public void AddNeeded(ItemDefinition item)
    {
        if (item.Equals(_sourceTokens.ArchiveItem))
        {
            return;
        }

        if (_sourceTokens.HasAnyRecipes(item))
        {
            throw new Exception($"Trying to add item from source as a needed item: {item}");
        }

        if (_tokens.InlineAssetsItem is not null && item.Equals(_tokens.InlineAssetsItem))
        {
            return;
        }

        if (_tokens.RemainderItem is not null && item.Equals(_tokens.RemainderItem))
        {
            return;
        }

        if (IsNeeded(item))
        {
            return;
        }

        _neededItems.Add(item);
    }

    public void RemoveNeeded(ItemDefinition item)
    {
        if (!IsNeeded(item))
        {
            return;
        }

        _neededItems.Remove(item);
    }
}