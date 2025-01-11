namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;

using ArchiveUtility;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;

public class SelectDeltasFromCatalog : Worker
{
    public Diff Diff { get; set; }

    public DeltaCatalog DeltaCatalog { get; set; }

    public SelectDeltasFromCatalog(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        Diff.SelectDeltasFromCatalog(DeltaCatalog);
    }
}