/**
* @file WriteDiffToDisk.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.IO;
using System.Threading;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class WriteDiffToDisk : Worker
{
    public string OutputFile { get; set; }

    public Diff Diff { get; set; }

    public WriteDiffToDisk(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        CheckForCancellation();

        var outputFile = Path.GetFullPath(OutputFile);

        if (File.Exists(outputFile))
        {
            Logger.LogInformation("Deleting existing file: {0}", outputFile);
            File.Delete(outputFile);
        }

        string outputDirectory = Path.GetDirectoryName(outputFile);
        if (!Directory.Exists(outputDirectory))
        {
            Directory.CreateDirectory(outputDirectory);
        }

        Logger.LogInformation("Writing diff to {0}", outputFile);

        string path = outputFile;
        DiffSerializer.WriteDiff(Diff, path);

        FileInfo diffFileInfo = new(outputFile);
        Logger.LogInformation($"Size: {diffFileInfo.Length:#,0.####} bytes");
    }
}
