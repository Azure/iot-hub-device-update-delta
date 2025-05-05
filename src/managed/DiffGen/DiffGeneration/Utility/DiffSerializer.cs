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
        session.SetTarget(diff.TargetItem);
        session.SetSource(diff.SourceItem);

        foreach (var recipe in diff.Recipes)
        {
            session.AddRecipe(recipe);
        }

        session.SetInlineAssets(diff.InlineAssetsPath);
        session.SetRemainder(diff.RemainderPath);

        session.WriteDiff(path);
    }
}
