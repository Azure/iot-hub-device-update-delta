/**
* @file Diff.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs;

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;

using ArchiveUtility;

public class Diff
{
    public ulong Version { get; set; } = 0;

    public ArchiveTokenization Tokens { get; set; } = new("Diff", "Standard");

    public string RemainderPath { get; set; }

    public string InlineAssetsPath { get; set; }

    public List<Recipe> DeltaRecipes { get; set; } = new();

    private HashSet<ItemDefinition> _neededItems = new();
    private HashSet<ItemDefinition> _solvedItems = new();

    private ArchiveTokenization _sourceTokens;
    private ArchiveTokenization _targetTokens;

    public Diff(ArchiveTokenization sourceTokens, ArchiveTokenization targetTokens)
    {
        Tokens.SourceItem = sourceTokens.ArchiveItem;
        Tokens.ArchiveItem = targetTokens.ArchiveItem;

        _sourceTokens = sourceTokens;
        _targetTokens = targetTokens;

        CheckForSolutions(null, targetTokens.ArchiveItem);
    }

    public void SelectDeltasFromCatalog(DeltaCatalog deltaCatalog)
    {
        _neededItems.Clear();
        CheckForSolutions(deltaCatalog, Tokens.ArchiveItem);
    }

    public IEnumerable<ItemDefinition> GetRemainderItems()
    {
        return _neededItems.Where(x => !Tokens.HasAnyRecipes(x));
    }

    public IEnumerable<ItemDefinition> GetSortedDeltaItems() => DeltaRecipes.Select(x => x.ItemIngredients.First()).Order();

    public bool IsNeededItem(ItemDefinition item) => _neededItems.Contains(item);

    private bool CheckForSolutions(DeltaCatalog deltaCatalog, ItemDefinition item)
    {
        if (_solvedItems.Contains(item))
        {
            return true;
        }

        if (deltaCatalog is null)
        {
            if (CheckForTrivialSolutions(item))
            {
                return true;
            }
        }
        else
        {
            if (CheckForDeltaSolutions(deltaCatalog, item))
            {
                return true;
            }
        }

        return UseTargetForwardRecipe(deltaCatalog, item);
    }

    private bool UseTargetForwardRecipe(DeltaCatalog deltaCatalog, ItemDefinition item)
    {
        // Otherwise, look at the recipes for the target and see check for solutions for the ingredients
        if (!_targetTokens.ForwardRecipes.TryGetValue(item, out var forwardRecipe))
        {
            _neededItems.Add(item);
            return false;
        }

        bool solved = true;

        foreach (var ingredient in forwardRecipe.ItemIngredients)
        {
            solved = CheckForSolutions(deltaCatalog, ingredient) && solved;
        }

        Tokens.AddForwardRecipe(forwardRecipe);

        if (solved)
        {
            _solvedItems.Add(item);
        }
        else
        {
            _neededItems.Add(item);
        }

        return solved;
    }

    private bool CheckForDeltaSolutions(DeltaCatalog deltaCatalog, ItemDefinition item)
    {
        List<Recipe> recipesToAdd = new();

        if (!deltaCatalog.TryGetRecipe(item, out var deltaRecipe))
        {
            return false;
        }

        recipesToAdd.Add(deltaRecipe);

        for (int iIngredient = 1; iIngredient < deltaRecipe.ItemIngredients.Count; iIngredient++)
        {
            var ingredient = deltaRecipe.ItemIngredients[iIngredient];

            if (!TryGetReverseSolution(ingredient, out var reverseSolution))
            {
                return false;
            }

            recipesToAdd.AddRange(reverseSolution);
        }

        DeltaRecipes.Add(deltaRecipe);

        foreach (var recipe in recipesToAdd)
        {
            Tokens.AddRecipe(recipe);
            _solvedItems.Add(recipe.Result);
        }

        return true;
    }

    private bool CheckForTrivialSolutions(ItemDefinition item)
    {
        // First, check if we have any context free solutions from the target for this item
        var targetRecipes = _targetTokens.GetRecipes(item);
        foreach (var recipe in targetRecipes)
        {
            if (recipe.ItemIngredients.Count == 0)
            {
                _solvedItems.Add(item);
                Tokens.AddRecipe(recipe);
                return true;
            }
        }

        // Next, try to get a recipe from the source
        var sourceRecipes = _sourceTokens.GetRecipes(item);
        foreach (var recipe in sourceRecipes)
        {
            if (recipe.ItemIngredients.Count == 0)
            {
                if (recipe.Result.CompareTo(item) != 0)
                {
                    throw new Exception("Found a recipe from the source for item, but result of recipe doesn't match item!");
                }

                Tokens.AddRecipe(recipe);
                _solvedItems.Add(item);
                return true;
            }
        }

        // Next, try getting a reverse recipe from the source!
        if (_sourceTokens.ReverseRecipes.TryGetValue(item, out var reverseRecipe))
        {
            if (TryGetReverseSolution(item, out var reverseSolution))
            {
                foreach (var solutionRecipe in reverseSolution)
                {
                    Tokens.AddRecipe(solutionRecipe);
                    _solvedItems.Add(solutionRecipe.Result);
                }

                return true;
            }
        }

        return false;
    }

    private bool TryGetReverseSolution(ItemDefinition item, out HashSet<Recipe> solution)
    {
        solution = new();

        if (_sourceTokens.ReverseRecipes.TryGetValue(item, out var reverseRecipe))
        {
            solution.Add(reverseRecipe);

            foreach (var ingredient in reverseRecipe.ItemIngredients)
            {
                if (ingredient.CompareTo(_sourceTokens.ArchiveItem) == 0)
                {
                    continue;
                }

                if (!TryGetReverseSolution(ingredient, out var ingredientSolution))
                {
                    return false;
                }

                solution.UnionWith(ingredientSolution);
            }

            return true;
        }

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
}