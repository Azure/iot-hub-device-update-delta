/**
 * @file ToolBasedDeltaBuilder.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Threading;
    using Microsoft.Extensions.Logging;

    using ArchiveUtility;

    abstract class ToolBasedDeltaBuilder : DeltaBuilder
    {
        public abstract string BinaryPath { get; set; }
        public abstract string FormatArgs(string source, string target, string delta);

        string TempDeltaPath(string path) => path + ".tmp";
        string BadDeltaPath(string path) => path + ".bad";

        protected void EnsureParentPath(string path)
        {
            string parentPath = Path.GetDirectoryName(path);
            if (!Directory.Exists(parentPath))
            {
                Directory.CreateDirectory(parentPath);
            }
        }

        public override bool Call(ILogger logger, string source, string target, bool targetIsCompressed, string delta)
        {
            if (File.Exists(delta))
            {
                return true;
            }

            var tempDelta = TempDeltaPath(delta);
            var badDiff = BadDeltaPath(delta);

            if (File.Exists(badDiff))
            {
                logger.LogInformation($"{this.GetType().Name}: Skipping file, detected bad delta: {badDiff}");
                return false;
            }

            EnsureParentPath(delta);

            int timeout = TimeoutValueGenerator.GetTimeoutValue(target, source);

            using (Process process = new())
            {
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.FileName = ProcessHelper.GetPathInRunningDirectory(BinaryPath);
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.Arguments = FormatArgs(source, target, tempDelta);

                process.StartAndReport();
                process.WaitForExit(timeout, CancellationToken);

                if (process.HasExited && process.ExitCode == 0)
                {
                    if (!File.Exists(tempDelta))
                    {
                        throw new Exception($"{this.GetType().Name}: didn't create file. Diff: {tempDelta}. BinaryPath: {process.StartInfo.FileName}, Args: {process.StartInfo.Arguments}");
                    }

                    const int max_retry_count = 5;
                    int retries = 0;

                    while (retries++ < max_retry_count)
                    {
                        try
                        {
                            File.Copy(tempDelta, delta);
                            File.Delete(tempDelta);
                            return true;
                        }
                        catch (IOException)
                        {
                            Thread.Sleep(1000);
                        }

                    }
                    throw new Exception($"Failed to move file {tempDelta} to {delta}");
                }

                logger.LogInformation($"{this.GetType().Name}: Failed. Creating bad diff: {badDiff}");
                Worker.CreateCookie(badDiff);

                if (!process.HasExited)
                {
                    logger.LogInformation($"{this.GetType().Name}: Killing diff process. Timeout of {timeout / 1000} seconds was exceeded.");
                    process.Kill(true);
                }

                if (File.Exists(tempDelta))
                {
                    File.Delete(tempDelta);
                }

                return false;
            }
        }
    }
}
