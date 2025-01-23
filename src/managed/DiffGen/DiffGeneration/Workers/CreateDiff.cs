/**
* @file CreateDiff.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class CreateDiff : Worker
{
    public Diff Diff { get; private set; }

    public ArchiveTokenization SourceTokens { get; private set; }

    public ArchiveTokenization TargetTokens { get; private set; }

    public CreateDiff(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        Diff diff = new(Logger, SourceTokens, TargetTokens, WorkingFolder);

        diff.Version = 1;

        Diff = diff;
    }
}
