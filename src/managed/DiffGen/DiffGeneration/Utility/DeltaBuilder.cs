/**
* @file DeltaBuilder.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Utility;

using System.Collections.Generic;
using System.IO;
using System.Threading;
using ArchiveUtility;
using Microsoft.Extensions.Logging;

public abstract class DeltaBuilder
{
    public CancellationToken CancellationToken { get; set; }

    public abstract bool Call(
        ILogger logger,
        ItemDefinition sourceItem,
        string sourceFile,
        ItemDefinition targetItem,
        string targetFile,
        string baseDeltaFile,
        out ItemDefinition deltaItem,
        out string deltaFile,
        out Recipe recipe);

    // Decorates a delta file path for this delta type (bsdiff, zstd, etc)
    public abstract string GetDecoratedFileName(string path);

    protected ILogger _logger { get; set; }

    public DeltaBuilder(ILogger logger)
    {
        _logger = logger;
    }

    // TODO: We don't want to diff headers against themselves because
    // they will zip well together in the remainder, but maybe
    // other files around this size may make sense to diff.
    // 513 is picked as a lot of headers are 512 bytes or so in size
    // and this lets us skip those, but take other files.
    // We can compare the diff savings compared to the cost in
    // diff chunk overhead to see if it's worth it, but still
    // will spend more time building due to this.
    public virtual ulong MINIMUM_SIZE { get; } = 513; // too small and we're not very likely to get a delta that's useful

    public virtual ulong MAXIMUM_SIZE { get; } = 250 * 1024 * 1024; // too big and run-time constraints are a problem

    public virtual bool ShouldDelta(ItemDefinition sourceItem, ItemDefinition targetItem)
    {
        return sourceItem.Length <= MAXIMUM_SIZE && sourceItem.Length >= MINIMUM_SIZE &&
            targetItem.Length <= MAXIMUM_SIZE && targetItem.Length >= MINIMUM_SIZE;
    }

    public string GetDecoratedPath(string path)
    {
        string parent = Path.GetDirectoryName(path);
        string file = Path.GetFileName(path);

        return Path.Combine(parent, GetDecoratedFileName(file));
    }
}
