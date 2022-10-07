/**
 * @file DeltaBuilder.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System.IO;
    using System.Threading;
    using ArchiveUtility;
    using Microsoft.Extensions.Logging;

    abstract class DeltaBuilder
    {
        public CancellationToken CancellationToken { get; set; }

        public abstract bool Call(ILogger logger, string source, string target, bool targetIsCompressed, string baseDeltaPath);
        public abstract DeltaRecipe MakeRecipe(RecipeParameter sourceParam, RecipeParameter deltaParam);

        // Decorates a delta file path for this delta type (bsdiff, zstd, etc)
        public abstract string GetDecoratedFileName(string path);

        // TODO: We don't want to diff headers against themselves because
        // they will zip well together in the remainder, but maybe 
        // other files around this size may make sense to diff.
        // 513 is picked as a lot of headers are 512 bytes or so in size
        // and this lets us skip those, but take other files.
        // We can compare the diff savings compared to the cost in
        // diff chunk overhead to see if it's worth it, but still
        // will spend more time building due to this.
        public virtual ulong MINIMUM_SIZE { get; } = 513; // too small and we're not very likely to get a delta that's useful
        public virtual ulong MAXIMUM_SIZE { get; } = (250 * 1024 * 1024); // too big and run-time constraints are a problem

        public virtual bool ShouldDelta(ArchiveItem source, ArchiveItem target)
        {
            return ((source.Length <= MAXIMUM_SIZE) && (source.Length >= MINIMUM_SIZE)) &&
                ((target.Length <= MAXIMUM_SIZE) && (target.Length >= MINIMUM_SIZE));
        }

        public string GetDecoratedPath(string path)
        {
            string parent = Path.GetDirectoryName(path);
            string file = Path.GetFileName(path);

            return Path.Combine(parent, GetDecoratedFileName(file));
        }
    }
}
