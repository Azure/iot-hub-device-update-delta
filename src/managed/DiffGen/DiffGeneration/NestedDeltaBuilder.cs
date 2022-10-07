/**
 * @file NestedDeltaBuilder.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.IO;
    using Microsoft.Extensions.Logging;

    using ArchiveUtility;

    class NestedDeltaBuilder : DeltaBuilder
    {
        public string WorkingFolder{ get;set;}
        public string LogFolder { get; set; }
        public bool KeepWorkingFolder { get; set; } = false;

        public override DeltaRecipe MakeRecipe(RecipeParameter sourceParam, RecipeParameter deltaParam)
        {
            return new ApplyNestedDiffRecipe(sourceParam, deltaParam);
        }

        public override string GetDecoratedFileName(string path) => "nested." + path;

        public override ulong MINIMUM_SIZE { get; } = (1ul * 1024 * 1024);
        public override ulong MAXIMUM_SIZE { get; } = (10ul * 1024 * 1024 * 1024); // 10 GB is quite large.

        public override bool Call(ILogger logger, string source, string target, bool targetIsCompressed, string delta)
        {
            bool isArchive;
            using (var stream = File.OpenRead(target))
            {
                // TODO replace with something more cheap instead of loading entire archive
                isArchive = ArchiveLoader.TryLoadArchive(stream, WorkingFolder, out ArchiveTokenization tokens);
            }

            // Run DiffBuilder recursively if the target payload can be loaded by ArchiveLoader
            if (!isArchive)
            {
                return false;
            }

            string deltaFileName = Path.GetFileName(delta);

            try
            {
                DiffBuilder.Execute(new DiffBuilder.Parameters()
                {
                    SourceFile = source,
                    TargetFile = target,
                    OutputFile = delta,
                    UseCopyTargetRecipes = !targetIsCompressed,
                    LogFolder = Path.Combine(LogFolder, deltaFileName),
                    WorkingFolder = Path.Combine(WorkingFolder, deltaFileName),
                    KeepWorkingFolder = KeepWorkingFolder,
                    CancellationToken = CancellationToken
                });
            }
            catch (Exception)
            {
                return false;
            }
            return true;
        }
    }
}
