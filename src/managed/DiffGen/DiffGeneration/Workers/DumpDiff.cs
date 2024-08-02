/**
* @file DumpDiff.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.IO;
using System.Threading;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class DumpDiff : Worker
{
    public string LogFolder { get; set; }

    public Diff Diff { get; set; }

    public DumpDiff(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        var path = Path.Combine(LogFolder, "diff.json");
        Logger.LogInformation($"Writing diff to: {path}");

        using (var stream = File.Create(path))
        {
            Diff.WriteJson(stream, true);
        }
    }
}
