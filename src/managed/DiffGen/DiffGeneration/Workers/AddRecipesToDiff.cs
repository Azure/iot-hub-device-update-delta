namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.Collections.Generic;
using System.Linq;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

internal class AddRecipesToDiff : Worker
{
    public Diff Diff { get; set; }

    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    public DeltaCatalog DeltaCatalog { get; set; }

    public AddRecipesToDiff(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        Logger.LogInformation("Adding Forward Recipes from Target to Diff.");

#pragma warning disable SA1010 // Opening square brackets should be spaced correctly
        List<ItemDefinition> items = [TargetTokens.ArchiveItem];

        int deltaRecipesAdded = 0;
        int reverseSourceRecipesAdded = 0;
        int forwardRecipesAdded = 0;
        int stillNeedRecipe = 0;

        while (items.Any())
        {
            List<ItemDefinition> nextItems = new();

            foreach (var item in items)
            {
                if (!Diff.IsNeeded(item))
                {
                    continue;
                }

                if (DeltaCatalog.TryGetRecipes(item, out var deltaRecipes) && deltaRecipes is not null && deltaRecipes.Any())
                {
                    Diff.AddRecipes(deltaRecipes, true);
                    deltaRecipesAdded += deltaRecipes.Count;

                    foreach (var deltaRecipe in deltaRecipes)
                    {
                        foreach (var ingredient in deltaRecipe.ItemIngredients)
                        {
                            List<Recipe> reverseRecipes = new();
                            if (Diff.TryGetReverseSolution(SourceTokens, ingredient, ref reverseRecipes))
                            {
                                Diff.AddRecipes(reverseRecipes, true);
                                reverseSourceRecipesAdded += reverseRecipes.Count;
                            }
                        }
                    }

                    continue;
                }

                var itemKey = item.WithoutNames();
                if (TargetTokens.ForwardRecipes.TryGetValue(itemKey, out var recipe))
                {
                    nextItems.AddRange(recipe.ItemIngredients);

                    Diff.AddRecipe(recipe, !recipe.ItemIngredients.Any());
                    forwardRecipesAdded++;
                }
                else
                {
                    stillNeedRecipe++;
                }
            }

            items = nextItems;
        }

        Logger.LogInformation("Added {DeltaRecipesAdded} delta recipes.", deltaRecipesAdded);
        Logger.LogInformation("Added {ReverseSourceRecipesAdded} reverse recipes from source.", reverseSourceRecipesAdded);
        Logger.LogInformation("Added {AddedCount} Forward Recipes from Target to Diff.", forwardRecipesAdded);
        Logger.LogInformation("{StillNeedRecipe} items still need a recipe.", stillNeedRecipe);
    }
}
