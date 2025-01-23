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
using System.ComponentModel.Design;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;

using ArchiveUtility;
using Microsoft.Extensions.Logging;

public class Diff
{
    public ulong Version { get; set; } = 0;

    public ArchiveTokenization Tokens { get; set; } = new("Diff", "Standard");

    public string RemainderPath { get; set; }

    public string InlineAssetsPath { get; set; }

    public List<Recipe> DeltaRecipes { get; set; } = new();

    private HashSet<ItemDefinition> _neededItems = new();
    private HashSet<ItemDefinition> _solvedItems = new();

    private ILogger _logger;
    private ArchiveTokenization _sourceTokens;
    private ArchiveTokenization _targetTokens;
    private string _workingFolder;

    public Diff(ILogger logger, ArchiveTokenization sourceTokens, ArchiveTokenization targetTokens, string workingFolder)
    {
        Tokens.SourceItem = sourceTokens.ArchiveItem;
        Tokens.ArchiveItem = targetTokens.ArchiveItem;

        _logger = logger;
        _sourceTokens = sourceTokens;
        _targetTokens = targetTokens;
        _workingFolder = workingFolder;

        CheckForSolutions(null, targetTokens.ArchiveItem);
    }

    public void SelectDeltasFromCatalog(DeltaCatalog deltaCatalog)
    {
        _neededItems.Clear();
        CheckForSolutions(deltaCatalog, Tokens.ArchiveItem);
    }

    public void ImportDeltasFromCatalog(DeltaCatalog deltaCatalog)
    {
        using var createSession = new DiffApi.DiffcSession();
        createSession.SetTarget(_targetTokens.ArchiveItem);

        foreach (var recipeSet in _targetTokens.Recipes)
        {
            foreach (var recipe in recipeSet.Value)
            {
                createSession.AddRecipe(recipe);
            }
        }

        foreach (var recipeSet in _sourceTokens.Recipes)
        {
            foreach (var recipe in recipeSet.Value)
            {
                createSession.AddRecipe(recipe);
            }
        }

        List<ItemDefinition> mockedItems = new();

        foreach (var recipe in deltaCatalog.Recipes)
        {
            createSession.AddRecipe(recipe.Value);
            mockedItems.Add(recipe.Value.ItemIngredients[0]);
        }

        using var applySession = createSession.NewApplySession();

        foreach (var item in _neededItems)
        {
            applySession.RequestItem(item);
        }

        mockedItems.Add(_sourceTokens.ArchiveItem);

        bool solvedAll = applySession.ProcessRequestedItemsEx(true, mockedItems);

        var selectedRecipesJson = Path.Combine(_workingFolder, "selectedRecipes.json");
        applySession.SaveSelectedRecipes(selectedRecipesJson);
        var selectedRecipes = LoadSelectedRecipes(selectedRecipesJson);

        if (selectedRecipes.Recipes is null)
        {
            return;
        }

        var selectedRecipeMap = selectedRecipes.Recipes.Select(x => new KeyValuePair<ItemDefinition, Recipe>(x.Result, x)).ToDictionary();

        _neededItems.Clear();
        _solvedItems.Clear();
        Tokens.ForwardRecipes.Clear();
        Tokens.ReverseRecipes.Clear();
        Tokens.ClearRecipes();

        CheckForSolutionsFromSelectedRecipes(selectedRecipeMap, _targetTokens.ArchiveItem);
    }

    public IEnumerable<ItemDefinition> GetNeededItems()
    {
        return _neededItems;
    }

    public IEnumerable<ItemDefinition> GetSortedDeltaItems() => DeltaRecipes.Select(x => x.ItemIngredients.First()).Order();

    public bool IsNeededItem(ItemDefinition item) => _neededItems.Contains(item);

    private bool CheckForSolutionsFromSelectedRecipes(Dictionary<ItemDefinition, Recipe> selectedRecipes, ItemDefinition item)
    {
        if (_solvedItems.Contains(item))
        {
            return true;
        }

        if (selectedRecipes.ContainsKey(item))
        {
            List<ItemDefinition> toAdd = [item];
            while (toAdd.Any())
            {
                List<ItemDefinition> newToAdd = new();

                foreach (var itemToAdd in toAdd)
                {
                    if (itemToAdd.CompareTo(_sourceTokens.ArchiveItem) == 0)
                    {
                        continue;
                    }

                    if (!selectedRecipes.ContainsKey(itemToAdd))
                    {
                        _logger.LogError("Selected Items doesn't contain recipe for item {itemToAdd}", itemToAdd.ToString());
                        throw new Exception($"Selected Items doesn't contain recipe for item {itemToAdd}");
                    }

                    var selectedRecipe = selectedRecipes[itemToAdd];

                    if (selectedRecipe.IsDeltaRecipe())
                    {
                        DeltaRecipes.Add(selectedRecipe);

                        if (selectedRecipe.HasDeltaBasis())
                        {
                            newToAdd.Add(selectedRecipe.GetDeltaBasis());
                        }
                    }
                    else
                    {
                        newToAdd.AddRange(selectedRecipe.ItemIngredients);
                    }

                    Tokens.AddRecipe(selectedRecipe);
                }

                toAdd = newToAdd;
            }

            return true;
        }

        // Otherwise, look at the recipes for the target and see check for solutions for the ingredients
        if (!_targetTokens.ForwardRecipes.TryGetValue(item, out var forwardRecipe))
        {
            _neededItems.Add(item);
            return false;
        }

        bool solved = true;

        foreach (var ingredient in forwardRecipe.ItemIngredients)
        {
            solved = CheckForSolutionsFromSelectedRecipes(selectedRecipes, ingredient) && solved;
        }

        Tokens.AddForwardRecipe(forwardRecipe);

        if (solved)
        {
            _solvedItems.Add(item);
        }

        return solved;
    }

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

    public static RecipeList LoadSelectedRecipes(string path)
    {
        var options = ArchiveTokenization.GetStandardJsonSerializerOptions();
        var jsonText = File.ReadAllText(path);
        var deserialized = JsonSerializer.Deserialize<RecipeList>(jsonText, options);
        return deserialized;
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