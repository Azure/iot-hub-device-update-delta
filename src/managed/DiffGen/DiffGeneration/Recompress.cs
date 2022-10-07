/**
 * @file Recompress.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using Microsoft.Extensions.Logging;
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading;

using ArchiveUtility;

namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    class Recompress : Worker
    {
        public string OriginalFile { get; set; }
        public string RecompressedFile { get; set; }
        public string SigningCommand { get; set; }
        public string StdOut { get; set; }
        public string StdErr { get; set; }

        public Recompress(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            CheckForCancellation();

            Logger.LogInformation($"Recompressing contents of {OriginalFile}");
            using (Process process = new())
            {
                string zstd_compress_file_path = ProcessHelper.GetPathInRunningDirectory("zstd_compress_file");
                string arguments = $"{OriginalFile} {RecompressedFile} {zstd_compress_file_path}";
                string pythonScriptPath;
                pythonScriptPath = ProcessHelper.GetPathInRunningDirectory("recompress_tool.py");
                if (!string.IsNullOrWhiteSpace(SigningCommand))
                {
                    arguments += $" \"{SigningCommand}\"";
                }

                process.StartInfo.UseShellExecute = false;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.RedirectStandardError = true;
                process.StartInfo.FileName = pythonScriptPath;
                process.StartInfo.Arguments = arguments;

                int timeout = TimeoutValueGenerator.GetTimeoutValue(OriginalFile);
                process.Start();
                process.WaitForExit(timeout);

                StdOut = process.StandardOutput.ReadToEnd();
                StdErr = process.StandardError.ReadToEnd();
            }
        }
    }
}
