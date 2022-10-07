/**
 * @file BsDiffDeltaBuilder.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System.Runtime.InteropServices;

    using ArchiveUtility;

    class BsDiffDeltaBuilder : ToolBasedDeltaBuilder
    {
        public static string BSDIFF_EXE_PATH
        {
            get
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    return "bsdiff";
                }

                return "bsdiff.exe";
            }
        }

        public override string BinaryPath { get; set; } = BSDIFF_EXE_PATH;

        public override string GetDecoratedFileName(string path) => "bsdiff." + path;

        public override string FormatArgs(string source, string target, string delta) => $"\"{source}\" \"{target}\" \"{delta}\"";

        public override DeltaRecipe MakeRecipe(RecipeParameter sourceParam, RecipeParameter deltaParam)
        {
            return new ApplyBsDiffRecipe(sourceParam, deltaParam);
        }
    }
}
