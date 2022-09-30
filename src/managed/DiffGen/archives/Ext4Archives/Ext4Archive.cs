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

    using Microsoft.Extensions.Logging;

    using ArchiveUtility;
    using System.Threading;

    public class Ext4Archive : IArchive
    {
        const int FIVE_MINUTES_IN_MILLISECONDS = 5 * 60 * 1000;

        public static string DUMPEXTFS_EXE_PATH
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

        public string ArchiveType => "Ext4";
        public string ArchiveSubtype => "base";

        private ArchiveLoaderContext Context;

        public Ext4Archive(ArchiveLoaderContext context)
        {
            Context = context;
        }

        private bool hasTriedProcessing = false;

        private ArchiveTokenization tokenization = null;

        const int CLEANUP_RETRIES = 5;
        const int ONE_SECOND_IN_MILLISECONDS = 1000;
        const int CLEANUP_RETRY_SLEEP = ONE_SECOND_IN_MILLISECONDS;

        void Cleanup(string path)
        {
            if (!File.Exists(path))
            {
                return;
            }

            for (int retry = 0; retry < CLEANUP_RETRIES; retry++)
            {
                try
                {
                    File.Delete(path);
                }
                catch (IOException)
                {
                    Thread.Sleep(CLEANUP_RETRY_SLEEP);
                }
            }
        }

        void TryTokenize()
        {
            hasTriedProcessing = true;

            string tempJsonPath = Path.GetTempFileName();

            try
            {
                using var fileFromStream = new FileFromStream(Context.Stream, Context.WorkingFolder);
                var archivePath = fileFromStream.Name;

                using (Process process = new())
                {
                    process.StartInfo.UseShellExecute = false;
                    process.StartInfo.CreateNoWindow = true;
                    process.StartInfo.RedirectStandardOutput = true;
                    process.StartInfo.FileName = ProcessHelper.GetPathInRunningDirectory(DUMPEXTFS_EXE_PATH);
                    process.StartInfo.Arguments = $"\"{archivePath}\" \"{tempJsonPath}\"";

                    process.StartAndReport();
                    process.WaitForExit(FIVE_MINUTES_IN_MILLISECONDS, Context.CancellationToken);

                    if (!process.HasExited)
                    {
                        throw new Exception($"{DUMPEXTFS_EXE_PATH} failed to load file within timeout of {FIVE_MINUTES_IN_MILLISECONDS} milliseconds.");
                    }

                    if (process.ExitCode != 0)
                    {
                        throw new Exception($"{DUMPEXTFS_EXE_PATH} failed to parse the provided file. Exit Code: {process.ExitCode}");
                    }

                    tokenization = ArchiveTokenization.FromJsonPath(tempJsonPath);
                }
            }
            catch (Exception e)
            {
                if (Context.Logger != null)
                {
                    Context.Logger.LogInformation($"[Tokenization of file as ext4 failed. Error: {e.Message}]");
                }
                tokenization = null;
            }
            finally
            {
                Cleanup(tempJsonPath);
            }
        }

        public bool IsMatchingFormat()
        {
            if (Context.TokenCache.ContainsKey(GetType()))
            {
                return true;
            }

            if (!hasTriedProcessing)
            {
                TryTokenize();
                if (tokenization != null)
                {
                    Context.TokenCache[GetType()] = tokenization;
                }
            }

            return tokenization != null;
        }

        public ArchiveTokenization Tokenize()
        {
            if (Context.TokenCache.ContainsKey(GetType()))
            {
                return Context.TokenCache[GetType()];
            }

            if (!hasTriedProcessing)
            {
                TryTokenize();
                if (tokenization != null)
                {
                    Context.TokenCache[GetType()] = tokenization;
                }
            }

            return tokenization;
        }
    }
}
