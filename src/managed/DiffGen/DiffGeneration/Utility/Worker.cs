/**
* @file Worker.cs
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

public abstract class Worker
{
    public ILogger Logger { get; set; }

    public string WorkingFolder { get; set; }

    protected CancellationToken CancellationToken { get; set; }

    public Worker(ILogger logger, string workingFolder, CancellationToken cancellationToken)
    {
        ArchiveLoaderContext.DefaultLogger = logger;

        Logger = logger;
        WorkingFolder = workingFolder;
        CancellationToken = cancellationToken;
    }

    public void Execute()
    {
        CheckForCancellation();

        Logger.LogInformation($"[ {GetType().Name} starting at {DateTime.Now} ]");

        Stopwatch stopWatch = new();
        stopWatch.Start();

        ExecuteInternal();

        stopWatch.Stop();

        TimeSpan elapsed = stopWatch.Elapsed;

        Logger.LogInformation($"[ {GetType().Name} finished at {DateTime.Now}. Elapsed Time: {elapsed} ]");

        Logger.LogInformation("Exit Code: {0}", Environment.ExitCode);
    }

    protected abstract void ExecuteInternal();

    protected void CheckForCancellation()
    {
        if (CancellationToken != CancellationToken.None)
        {
            CancellationToken.ThrowIfCancellationRequested();
        }
    }

    public static void CreateCookie(string path)
    {
        using (var writer = File.CreateText(path))
        {
        }
    }
}
