/**
* @file DiffSerializer.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Utility;

using ArchiveUtility;

public class DiffSerializer
{
    public static void WriteDiff(Diff diff, string path)
    {
        using var session = new DiffApi.DiffcSession();
        session.SetTarget(diff.Tokens.ArchiveItem);
        session.SetSource(diff.Tokens.SourceItem);

        foreach (var entry in diff.Tokens.Recipes)
        {
            var recipes = entry.Value;
            foreach (var recipe in recipes)
            {
                session.AddRecipe(recipe);
            }
        }

        session.SetInlineAssets(diff.InlineAssetsPath);
        session.SetRemainder(diff.RemainderPath);

        session.WriteDiff(path);
    }
}
