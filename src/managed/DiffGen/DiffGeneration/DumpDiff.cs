/**
 * @file DumpDiff.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System.IO;
    using System.Threading;
    using Microsoft.Extensions.Logging;

    class DumpDiff : Worker
    {
        public string LogFolder { get; set; }
        public Diff Diff { get; set; }

        public DumpDiff(CancellationToken cancellationToken) :
            base(cancellationToken)
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
}
