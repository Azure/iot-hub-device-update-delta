﻿namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    public class RecipeCatalog
    {
        private Dictionary<ItemDefinition, HashSet<Recipe>> _recipes = new();

        // The get makes a copy. If we modify the copy (via AddRanges, Add, Clear, etc), this
        // will have no effect on the underlying data
        public Dictionary<ItemDefinition, HashSet<Recipe>> Entries { get => _recipes; }

        public void ClearRecipes() => _recipes.Clear();

        private static bool IsSliceOfItem(Recipe recipe, ItemDefinition item) => recipe.Name.Equals("slice") && recipe.ItemIngredients[0].Equals(item);

        public IEnumerable<Recipe> GetSlicesOf(ItemDefinition item)
        {
            return _recipes.SelectMany(x => x.Value.Where(recipe => IsSliceOfItem(recipe, item)));
        }

        public IEnumerable<Recipe> GetAllRecipes()
        {
            List<Recipe> allRecipes = new();

            foreach (var entry in _recipes)
            {
                var recipes = entry.Value;
                foreach (var recipe in recipes)
                {
                    allRecipes.Add(recipe);
                }
            }

            return allRecipes;
        }

        public IEnumerable<Recipe> GetRecipesUsing(ItemDefinition usingItem)
        {
            List<Recipe> recipesUsing = new();
            if (usingItem == null)
            {
                return recipesUsing;
            }

            foreach (var entry in _recipes)
            {
                var recipes = entry.Value;
                foreach (var recipe in recipes)
                {
                    if (recipe.ItemIngredients.Count == 1 && recipe.ItemIngredients[0].Equals(usingItem))
                    {
                        recipesUsing.Add(recipe);
                    }
                }
            }

            return recipesUsing;
        }

        public void AddRecipe(Recipe recipe)
        {
            if (!TryAddRecipe(recipe))
            {
                throw new Exception($"Couldn't validate recipe being added for result: {recipe.Result}");
            }
        }

        public bool TryAddRecipe(Recipe recipe)
        {
            if (!_recipes.ContainsKey(recipe.Result))
            {
                _recipes.Add(recipe.Result, new());
            }

            var recipes = _recipes[recipe.Result];

            recipes.Add(recipe);
            return true;
        }

        public HashSet<Recipe> GetRecipes(ItemDefinition item)
        {
            if (_recipes.ContainsKey(item))
            {
                return _recipes[item];
            }

            return new();
        }

        public void PruneInvalidRecipes()
        {
            foreach (var entry in _recipes)
            {
                var recipes = entry.Value;

                List<Recipe> invalid = new();
                Recipe winner = null;
                foreach (var recipe in recipes)
                {
                    if (recipe.ItemIngredients.Count == 0)
                    {
                        winner = recipe;
                        continue;
                    }
                }

                foreach (var recipe in invalid)
                {
                    recipes.Remove(recipe);
                }
            }
        }

        public bool HasAnyRecipes(ItemDefinition item)
        {
            return _recipes.ContainsKey(item) && _recipes[item].Count > 0;
        }

        public bool HasRecipe(Recipe recipe)
        {
            if (!_recipes.ContainsKey(recipe.Result))
            {
                return false;
            }

            return _recipes[recipe.Result].Contains(recipe);
        }
    }
}
