/**
* @file ToolBasedDeltaBuilder.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Utility;

using System;
using System.Diagnostics;
using System.IO;
using System.Threading;

using ArchiveUtility;
using Microsoft.Extensions.Logging;

public abstract class ToolBasedDeltaBuilder : DeltaBuilder
{
    public abstract string BinaryPath { get; set; }

    public abstract string DecompressionRecipeName { get; set; }

    public abstract string FormatArgs(string source, string target, string delta);

    private string TempDeltaPath(string path) => path + ".tmp";

    private string BadDeltaPath(string path) => path + ".bad";

    public ToolBasedDeltaBuilder(ILogger logger)
        : base(logger)
    {
    }

    protected void EnsureParentPath(string path)
    {
        string parentPath = Path.GetDirectoryName(path);
        if (!Directory.Exists(parentPath))
        {
            Directory.CreateDirectory(parentPath);
        }
    }

    public Recipe CreateDecompressionRecipe(ItemDefinition result, ItemDefinition deltaItem, ItemDefinition sourceItem) =>
        new(DecompressionRecipeName, result, new(), new() { deltaItem, sourceItem });

    public override bool Call(
        ILogger logger,
        ItemDefinition sourceItem,
        string sourceFile,
        ItemDefinition targetItem,
        string targetFile,
        string baseDeltaFile,
        out ItemDefinition deltaItem,
        out string deltaFile,
        out Recipe recipe)
    {
        deltaFile = GetDecoratedPath(baseDeltaFile);

        var tempDelta = TempDeltaPath(deltaFile);
        var badDiff = BadDeltaPath(deltaFile);

        if (File.Exists(badDiff))
        {
            logger.LogDebug($"{GetType().Name}: Skipping file, detected bad delta: {badDiff}");
            recipe = null;
            deltaItem = null;
            return false;
        }

        if (File.Exists(deltaFile))
        {
            deltaItem = ItemDefinition.FromFile(deltaFile);

            if (deltaItem.Length == 0)
            {
                File.Delete(deltaFile);
            }
            else if (deltaItem.Length >= targetItem.Length)
            {
                Worker.CreateCookie(badDiff);

                logger.LogDebug("Skipping delta - length of {0} >= the target length of {1}", deltaItem.Length, targetItem.Length);
                recipe = null;
                deltaItem = null;
                return false;
            }
            else
            {
                recipe = CreateDecompressionRecipe(targetItem, deltaItem, sourceItem);
                return true;
            }
        }

        EnsureParentPath(deltaFile);

        int timeout = TimeoutValueGenerator.GetTimeoutValue(sourceFile, targetFile);

        using (Process process = new())
        {
            process.StartInfo.UseShellExecute = false;
            process.StartInfo.FileName = ProcessHelper.GetPathInRunningDirectory(BinaryPath);
            process.StartInfo.CreateNoWindow = true;
            process.StartInfo.Arguments = FormatArgs(sourceFile, targetFile, tempDelta);

            process.StartAndReport();
            process.WaitForExit(timeout, CancellationToken);

            if (process.HasExited && process.ExitCode == 0)
            {
                if (!File.Exists(tempDelta))
                {
                    throw new Exception($"{GetType().Name}: didn't create file. Diff: {tempDelta}. BinaryPath: {process.StartInfo.FileName}, Args: {process.StartInfo.Arguments}");
                }

                MoveFileWithRetries(tempDelta, deltaFile);

                deltaItem = ItemDefinition.FromFile(deltaFile);

                if (deltaItem.Length >= targetItem.Length)
                {
                    Worker.CreateCookie(badDiff);

                    logger.LogDebug("Skipping delta - length of {0} >= the target length of {1}", deltaItem.Length, targetItem.Length);
                    recipe = null;
                    deltaItem = null;
                    return false;
                }

                recipe = CreateDecompressionRecipe(targetItem, deltaItem, sourceItem);
                return true;
            }

            logger.LogDebug($"{GetType().Name}: Failed. Creating bad diff: {badDiff}");
            Worker.CreateCookie(badDiff);

            if (!process.HasExited)
            {
                logger.LogDebug($"{GetType().Name}: Killing diff process. Timeout of {timeout / 1000} seconds was exceeded.");
                process.Kill(true);
            }

            if (File.Exists(tempDelta))
            {
                File.Delete(tempDelta);
            }

            recipe = null;
            deltaItem = null;
            return false;
        }
    }

    private static void MoveFileWithRetries(string source, string destination)
    {
        const int max_retry_count = 5;
        int retries = 0;
        Exception lastException = null;

        while (retries++ < max_retry_count)
        {
            try
            {
                File.Move(source, destination);
                return;
            }
            catch (IOException e)
            {
                lastException = e;
                Thread.Sleep(1000);
            }
        }

        throw new Exception($"Failed to move {source} to {destination}. {lastException.Message}", lastException);
    }
}