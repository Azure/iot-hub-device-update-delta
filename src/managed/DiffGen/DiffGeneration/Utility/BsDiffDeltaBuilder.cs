/**
* @file BsDiffDeltaBuilder.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Utility;

using System.Runtime.InteropServices;

using Microsoft.Extensions.Logging;

public class BsDiffDeltaBuilder : ToolBasedDeltaBuilder
{
    public static string BsdiffExePath
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

    public override string BinaryPath { get; set; } = BsdiffExePath;

    public override string DecompressionRecipeName { get; set; } = "bspatch_decompression";

    public override string GetDecoratedFileName(string path) => path + ".bsdiff";

    public override string FormatArgs(string source, string target, string delta) => $"\"{source}\" \"{target}\" \"{delta}\"";

    public BsDiffDeltaBuilder(ILogger logger)
        : base(logger)
    {
    }
}
