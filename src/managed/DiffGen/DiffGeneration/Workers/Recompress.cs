/**
 * @file Recompress.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.Diagnostics;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class Recompress : Worker
{
    public string OriginalFile { get; set; }

    public string RecompressedFile { get; set; }

    public string SigningCommand { get; set; }

    public string StdOut { get; set; }

    public string StdErr { get; set; }

    public Recompress(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        CheckForCancellation();

        Logger.LogInformation($"Recompressing contents of {OriginalFile}");
        using (Process process = new())
        {
            string recompress_exe = ProcessHelper.GetPathInRunningDirectory("recompress");
            string arguments = $"swu \"{OriginalFile}\" \"{RecompressedFile}\"";
            if (!string.IsNullOrWhiteSpace(SigningCommand))
            {
                arguments += $" \"{SigningCommand}\"";
            }

            Logger.LogInformation($"Executing: {recompress_exe} with arguments: {arguments}");

            process.StartInfo.UseShellExecute = false;
            process.StartInfo.CreateNoWindow = true;
            process.StartInfo.RedirectStandardOutput = true;
            process.StartInfo.RedirectStandardError = true;
            process.StartInfo.FileName = recompress_exe;
            process.StartInfo.Arguments = arguments;

            int timeout = TimeoutValueGenerator.GetTimeoutValue(OriginalFile);
            process.Start();
            process.WaitForExit(timeout);

            StdOut = process.StandardOutput.ReadToEnd();
            StdErr = process.StandardError.ReadToEnd();
        }
    }
}
