/**
 * @file Worker.cs
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

    public abstract class Worker
    {
        public ILogger Logger { get; set; }
        public string WorkingFolder { get; set; }
        protected CancellationToken CancellationToken { get; set; }

        public Worker(CancellationToken cancellationToken)
        {
            CancellationToken = cancellationToken;
        }

        public void Execute()
        {
            CheckForCancellation();

            Logger.LogInformation($"[ {this.GetType().Name} starting at {DateTime.Now} ]");

            Stopwatch stopWatch = new();
            stopWatch.Start();

            ExecuteInternal();

            stopWatch.Stop();

            TimeSpan elapsed = stopWatch.Elapsed;

            Logger.LogInformation($"[ {this.GetType().Name} finished at {DateTime.Now}. Elapsed Time: {elapsed} ]");
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
}
