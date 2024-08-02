/**
 * @file Ext4Archive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Ext4Archives
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Threading;

    using ArchiveUtility;
    using Microsoft.Extensions.Logging;

    public class Ext4Archive : ArchiveImpl
    {
        private const int FiveMinutesInMilliseconds = 5 * 60 * 1000;
        private const int CleanupRetries = 5;
        private const int OneSecondInMilliseconds = 1000;
        private const int CleanupRetrySleep = OneSecondInMilliseconds;

        public static string DumpextfsPath
        {
            get
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    return "dumpextfs";
                }

                return "dumpextfs.exe";
            }
        }

        public override string ArchiveType => "ext4";

        public override string ArchiveSubtype => "base";

        public Ext4Archive(ArchiveLoaderContext context)
            : base(context)
        {
        }

        private void Cleanup(string path)
        {
            if (!File.Exists(path))
            {
                return;
            }

            for (int retry = 0; retry < CleanupRetries; retry++)
            {
                try
                {
                    File.Delete(path);
                }
                catch (IOException)
                {
                    Thread.Sleep(CleanupRetrySleep);
                }
            }
        }

        public override bool TryTokenize(ItemDefinition archiveItem, out ArchiveTokenization tokens)
        {
            string jsonFile = archiveItem.GetExtractionPath(Context.WorkingFolder) + ".ext4.json";

            try
            {
                using var fileFromStream = new FileFromStream(Context.Stream, Context.WorkingFolder);
                var archivePath = fileFromStream.Name;

                using (Process process = new())
                {
                    process.StartInfo.UseShellExecute = false;
                    process.StartInfo.CreateNoWindow = true;
                    process.StartInfo.RedirectStandardOutput = true;
                    process.StartInfo.FileName = ProcessHelper.GetPathInRunningDirectory(DumpextfsPath);
                    process.StartInfo.Arguments = $"\"{archivePath}\" \"{jsonFile}\"";
                    process.StartInfo.RedirectStandardOutput = true;
                    process.StartInfo.RedirectStandardError = true;

                    process.OutputDataReceived += (sender, e) =>
                    {
                        if (e.Data is not null)
                        {
                            Context.Logger.LogInformation(e.Data);
                        }
                    };

                    process.ErrorDataReceived += (sender, e) =>
                    {
                        if (e.Data is not null)
                        {
                            Context.Logger.LogError(e.Data);
                        }
                    };

                    process.StartAndReport();
                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();
                    process.WaitForExit(FiveMinutesInMilliseconds, Context.CancellationToken);

                    if (!process.HasExited)
                    {
                        throw new Exception($"{DumpextfsPath} failed to load file within timeout of {FiveMinutesInMilliseconds} milliseconds.");
                    }

                    if (process.ExitCode != 0)
                    {
                        throw new Exception($"{DumpextfsPath} failed to parse the provided file: {archivePath}. Exit Code: {process.ExitCode}");
                    }

                    tokens = ArchiveTokenization.FromJsonPath(jsonFile);
                }
            }
            catch (Exception e)
            {
                if (Context.Logger is not null)
                {
                    Context.Logger.Log(Context.LogLevel, "[Tokenization of file as ext4 failed. Error: {msg}]", e.Message);
                    if (File.Exists(jsonFile))
                    {
                        Context.Logger.Log(Context.LogLevel, "[ext4 JsonFile: {jsonFile}]", jsonFile);
                    }
                }

                tokens = null;
                return false;
            }

            return true;
        }
    }
}
