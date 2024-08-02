/**
* @file ZstdDeltaBuilder.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Utility;

using System.Runtime.InteropServices;

using Microsoft.Extensions.Logging;

public class ZstdDeltaBuilder : ToolBasedDeltaBuilder
{
    public static string ZstdCompressExeFilePath
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

    public override string BinaryPath { get; set; } = ZstdCompressExeFilePath;

    public override string DecompressionRecipeName { get; set; } = "zstd_decompression";

    public override string GetDecoratedFileName(string path) => path + ".zstd_delta";

    public override string FormatArgs(string source, string target, string delta) => $@"""{target}"" ""{source}"" ""{delta}""";

    public ZstdDeltaBuilder(ILogger logger)
        : base(logger)
    {
    }
}
