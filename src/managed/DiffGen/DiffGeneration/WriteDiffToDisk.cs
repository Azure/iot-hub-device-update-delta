/**
 * @file WriteDiffToDisk.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System.IO;
    using System.Threading;

    using Microsoft.Extensions.Logging;

    class WriteDiffToDisk : Worker
    {
        public string OutputFile { get; set; }
        public Diff Diff { get; set; }

        public WriteDiffToDisk(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            CheckForCancellation();

            if (File.Exists(OutputFile))
            {
                Logger.LogInformation("Deleting existing file: {0}", OutputFile);
                File.Delete(OutputFile);
            }

            string outputDirectory = Path.GetDirectoryName(OutputFile);
            if (!Directory.Exists(outputDirectory))
            {
                Directory.CreateDirectory(outputDirectory);
            }

            Logger.LogInformation("Writing diff to {0}", OutputFile);

            string path = OutputFile;
            DiffSerializer.WriteDiff(Diff, path);

            FileInfo diffFileInfo = new(OutputFile);
            Logger.LogInformation($"Size: {diffFileInfo.Length:#,0.####} bytes");
        }
    }
}
