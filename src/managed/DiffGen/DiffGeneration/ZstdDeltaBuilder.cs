/**
 * @file ZstdDeltaBuilder.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System.Runtime.InteropServices;

    using ArchiveUtility;
    class ZstdDeltaBuilder : ToolBasedDeltaBuilder
    {
        public static string ZSTD_COMPRESS_FILE_EXE_PATH
        {
            get
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    return "zstd_compress_file";
                }

                return "zstd_compress_file.exe";
            }
        }

        public override string BinaryPath { get; set; } = ZSTD_COMPRESS_FILE_EXE_PATH;

        public override string GetDecoratedFileName(string path) => "zstd_delta." + path;
        public override string FormatArgs(string source, string target, string delta) => $@"""{target}"" ""{source}"" ""{delta}""";
        public override DeltaRecipe MakeRecipe(RecipeParameter sourceParam, RecipeParameter deltaParam)
        {
            return new ApplyZstdDeltaRecipe(sourceParam, deltaParam);
        }
    }
}
