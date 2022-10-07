/**
 * @file DiffBuilder.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using Microsoft.Extensions.Logging;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    using ArchiveUtility;
    using Ext4Archives;

    public class DiffBuilder : Worker
    {
        public enum ParametersStatus
        {
            Ok,
            MissingBinaries,
            MissingParameters,
            MissingFiles,
        }

        public class Parameters
        {
            public string SourceFile { get; set; }
            public string TargetFile { get; set; }
            public string OutputFile { get; set; }
            public string LogFolder { get; set; }
            public string WorkingFolder { get; set; }
            public string RecompressedTargetFile { get; set; }
            public string SigningCommand { get; set; }
            public bool KeepWorkingFolder { get; set; } = false;
            public bool UseCopyTargetRecipes { get; set; } = true;
            public CancellationToken CancellationToken { get; set; } = CancellationToken.None;

            static bool IsBinaryMissingLinux(string name)
            {
                try
                {
                    using (Process process = new())
                    {
                        process.StartInfo.UseShellExecute = false;
                        process.StartInfo.CreateNoWindow = true;
                        process.StartInfo.RedirectStandardOutput = true;
                        process.StartInfo.RedirectStandardError = true;
                        process.StartInfo.FileName = ProcessHelper.GetPathInRunningDirectory(name);

                        Trace.TraceInformation($"Trying to detect if {name} exists by running {process.StartInfo.FileName}");

                        const int TEN_SECONDS = 10000;
                        process.Start();
                        process.WaitForExit(TEN_SECONDS);

                        //ten seconds should be enough
                        return !process.HasExited;
                    }
                }
                catch (Exception)
                {
                    return true;
                }
            }
            [DllImport("shlwapi.dll", CharSet = CharSet.Unicode, SetLastError = false)]
            static extern bool PathFindOnPath([In, Out] StringBuilder pszFile, [In] String[] ppszOtherDirs);

            static bool IsBinaryMissingWindows(string name)
            {
                const int MAX_PATH = 260;

                string pathInRunningFolder = ProcessHelper.GetPathInRunningDirectory(name);

                if (File.Exists(pathInRunningFolder))
                {
                    return false;
                }

                return !PathFindOnPath(new StringBuilder(name, MAX_PATH), null);
            }

            static bool IsBinaryMissing(string name)
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    return IsBinaryMissingLinux(name);
                }

                return IsBinaryMissingWindows(name);
            }

            public static string[] EnumerateMissingBinaries()
            {
                List<string> requiredBinaries = new()
                {
                    Ext4Archive.DUMPEXTFS_EXE_PATH,
                    ZstdDeltaBuilder.ZSTD_COMPRESS_FILE_EXE_PATH,
                    BsDiffDeltaBuilder.BSDIFF_EXE_PATH,
                };

                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    var missingBinaries = requiredBinaries.Where(b => IsBinaryMissing(b)).ToList();
                    if (!ADUCreate.CanOpenSession())
                    {
                        missingBinaries.Add(ADUCreate.ADUDIFFAPI_DLL);
                    }
                    return missingBinaries.ToArray();
                }
                else
                {
                    requiredBinaries.Add(ADUCreate.ADUDIFFAPI_DLL_WINDOWS);
                    requiredBinaries.AddRange(ADUCreate.ADUDIFFAPI_DEPENDENCIES);
                    return requiredBinaries.Where(b => IsBinaryMissing(b)).ToArray();
                }
            }
            public string[] EnumerateMissingParameters()
            {
                List<string> missingParameters = new();

                if (string.IsNullOrEmpty(SourceFile))
                {
                    missingParameters.Add("SourceFile");
                }

                if (string.IsNullOrEmpty(TargetFile))
                {
                    missingParameters.Add("TargetFile");
                }

                if (string.IsNullOrEmpty(OutputFile))
                {
                    missingParameters.Add("OutputFile");
                }

                if (string.IsNullOrEmpty(LogFolder))
                {
                    missingParameters.Add("LogFolder");
                }

                return missingParameters.ToArray();
            }

            public string[] EnumerateMissingFiles()
            {
                return (new List<string> { SourceFile, TargetFile })
                    .Where(f => !File.Exists(f))
                    .ToArray();
            }

            public ParametersStatus Validate(out string[] issues)
            {
                issues = EnumerateMissingBinaries();
                if (issues.Length > 0)
                {
                    return ParametersStatus.MissingBinaries;
                }

                issues = EnumerateMissingParameters();
                if (issues.Length > 0)
                {
                    return ParametersStatus.MissingParameters;
                }

                issues = EnumerateMissingFiles();
                if (issues.Length > 0)
                {
                    return ParametersStatus.MissingFiles;
                }

                issues = Array.Empty<string>();
                return ParametersStatus.Ok;
            }
        }

        public static Task ExecuteAsync(Parameters parameters)
        {
            CancellationToken cancellationToken = parameters.CancellationToken;
            Action work = () =>
            {
                if (cancellationToken != CancellationToken.None)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                }

                Execute(parameters);
            };

            if (cancellationToken == CancellationToken.None)
            {
                return Task.Run(work);
            }
            else
            {
                return Task.Run(work, cancellationToken);
            }
        }

        public static void Execute(Parameters parameters)
        {
            Directory.CreateDirectory(parameters.LogFolder);
            Directory.CreateDirectory(parameters.WorkingFolder);

            string ProcessesReportFile = Path.Combine(parameters.LogFolder, "ProcessesReport.json");
            if (File.Exists(ProcessesReportFile))
            {
                File.Delete(ProcessesReportFile);
            }
            ProcessHelper.SetProcessesReportFilePath(ProcessesReportFile);

            string logPath = Path.Combine(parameters.LogFolder, "DiffBuilder.log");

            using var logger = new DiffLogger(logPath);

            DiffBuilder builder = new(parameters.CancellationToken)
            {
                Logger = logger,
                SourceFile = parameters.SourceFile,
                TargetFile = parameters.TargetFile,
                OutputFile = parameters.OutputFile,
                LogFolder = parameters.LogFolder,
                WorkingFolder = parameters.WorkingFolder,
                RecompressedTargetFile = parameters.RecompressedTargetFile,
                SigningCommand = parameters.SigningCommand,
                KeepWorkingFolder = parameters.KeepWorkingFolder,
                UseCopyTargetRecipes = parameters.UseCopyTargetRecipes
            };

            builder.Execute();
        }

        #region("Input Properties")
        public string SourceFile { get; private set; }
        public string TargetFile { get; private set; }
        public string OutputFile { get; private set; }
        public string LogFolder { get; private set; }
        public bool KeepWorkingFolder { get; set; }
        public string RecompressedTargetFile { get; private set; }
        public string SigningCommand { get; private set; }
        public bool UsePayloadDevicePaths { get; set; } = false;
        public bool UseSourceFile { get; set; } = true;
        public bool UseCopyTargetRecipes { get; set; } = true;
        #endregion

        #region("Internal Properties")
        private Diff diff = null;
        #endregion

        #region("Output Properties")
        public ArchiveTokenization SourceTokens { get; private set; }
        public ArchiveTokenization TargetTokens { get; private set; }
        public ArchiveTokenization DeltaTokens { get; private set; }
        public ulong TotalBytesCopiedFromTarget { get; private set; }
        public int ChunkCountCopiedFromTarget { get; private set; }
        public ulong TotalBytesCopiedFromSourceChunks { get; private set; }
        public int ChunkCountCopiedFromSourceChunks { get; private set; }
        public ulong TotalBytesCopiedFromIdenticalPayload { get; private set; }
        public int ChunkCountCopiedFromIdenticalPayload { get; private set; }
        public ulong TotalBytesFromDeltaBasedRecipes { get; private set; }
        public int ChunkCountFromDeltaBasedRecipes { get; private set; }
        #endregion

        public DiffBuilder(CancellationToken cancellationToken):
            base(cancellationToken)
        {
        }

        #region("Parameter Methods")
        void LogParameters()
        {
            if (Logger is DiffLogger)
            {
                Logger.LogInformation("Log File Path: {0}", (Logger as DiffLogger).LogPath);
            }

            Logger.LogInformation($"SourceFile: {SourceFile}");
            Logger.LogInformation($"TargetFile: {TargetFile}");
            Logger.LogInformation($"OutputFile: {OutputFile}");
            Logger.LogInformation($"WorkingFolder: {WorkingFolder}");
            Logger.LogInformation($"LogFolder: {LogFolder}");

            if (!string.IsNullOrEmpty(RecompressedTargetFile))
            {
                Logger.LogInformation($"RecompressedTargetFile: {RecompressedTargetFile}");
            }

            if (!string.IsNullOrEmpty(SigningCommand))
            {
                Logger.LogInformation($"SigningCommand: {SigningCommand}");
            }
        }

        public void EnsureWorkingFolder()
        {
            if (string.IsNullOrEmpty(WorkingFolder))
            {
                do
                {
                    WorkingFolder = Path.Combine(Path.GetTempPath(), "ADUDiffGeneration", Path.GetRandomFileName());
                }
                while (Directory.Exists(WorkingFolder));
            }
            else if (!KeepWorkingFolder)
            {
                Directory.Delete(WorkingFolder, true);
            }

            Directory.CreateDirectory(WorkingFolder);
        }
        #endregion

        #region("Worker Calls")
        void Recompress()
        {
            if (string.IsNullOrEmpty(RecompressedTargetFile))
            {
                return;
            }

            Recompress worker = new(CancellationToken)
            {
                Logger = Logger,
                WorkingFolder = WorkingFolder,
                OriginalFile = TargetFile,
                RecompressedFile = RecompressedTargetFile,
                SigningCommand = SigningCommand
            };
            worker.Execute();

            WriteToLogFile(worker.StdOut, "recompression.stdout.txt");
            WriteToLogFile(worker.StdErr, "recompression.stderr.txt");

            if (!File.Exists(RecompressedTargetFile))
            {
                throw new Exception($"Failed to create file {RecompressedTargetFile}\nError Message:\n{worker.StdErr}");
            }

            TargetFile = RecompressedTargetFile;
        }

        void Expand()
        {
            Expand worker = new(CancellationToken)
            {
                Logger = Logger,
                WorkingFolder = WorkingFolder,
                SourceFile = SourceFile,
                TargetFile = TargetFile
            };
            worker.Execute();

            SourceTokens = worker.SourceTokens;
            TargetTokens = worker.TargetTokens;

            File.Copy(worker.SourceTokensFile, Path.Combine(LogFolder, "source.archive.json"), true);
            File.Copy(worker.TargetTokensFile, Path.Combine(LogFolder, "target.archive.json"), true);
        }

        void CreateDeltas()
        {
            var worker = new CreateDeltas(CancellationToken)
            {
                Logger = Logger,
                LogFolder = LogFolder,
                WorkingFolder = WorkingFolder,
                SourceTokens = SourceTokens,
                TargetTokens = TargetTokens,
                UsePayloadDevicePaths = UsePayloadDevicePaths,
                UseSourceFile = UseSourceFile,
                KeepWorkingFolder = KeepWorkingFolder,
                SourceFile = SourceFile,
                TargetFile = TargetFile,
                Diff = diff
            };

            worker.Execute();

            DeltaTokens = worker.DeltaTokens;

            File.Copy(worker.DeltaTokensFile, Path.Combine(LogFolder, "deltas.json"), true);
        }

        void CreateDiff()
        {
            var worker = new CreateDiff(CancellationToken)
            {
                Logger = Logger,
                TargetTokens = TargetTokens,
                SourceFile = SourceFile,
                TargetFile = TargetFile
            };

            worker.Execute();

            diff = worker.Diff;

            Logger.LogInformation($"{diff.Chunks.Count} chunks in diff to process.");
        }

        void AddCopiesForDuplicateChunks()
        {
            var worker = new AddCopiesForDuplicateChunks(CancellationToken) { Logger = Logger, Diff = diff };

            worker.Execute();

            TotalBytesCopiedFromTarget = worker.TotalBytesCopiedFromTarget;
            ChunkCountCopiedFromTarget = worker.ChunkCountCopiedFromTarget;

            Logger.LogInformation($"{ChunkCountCopiedFromTarget} chunks ({TotalBytesCopiedFromTarget:#,0.####} bytes) copied from target archive.");
            LogChunksMissingRecipe(Logger, diff);
        }

        void AddSourceCopyRecipes()
        {
            var worker = new AddSourceCopyRecipes(CancellationToken)
            {
                Logger = Logger,
                Diff = diff,
                SourceTokens = SourceTokens,
                UseSourceFile = UseSourceFile
            };

            worker.Execute();

            TotalBytesCopiedFromSourceChunks = worker.TotalBytesCopiedFromSourceChunks;
            ChunkCountCopiedFromSourceChunks = worker.ChunkCountCopiedFromSourceChunks;

            Logger.LogInformation($"{ChunkCountCopiedFromSourceChunks} chunks ({TotalBytesCopiedFromSourceChunks:#,0.####}) bytes copied from source archive chunks.");
            LogChunksMissingRecipe(Logger, diff);
        }

        void AddIdenticalPayloadBasedRecipes()
        {
            var worker = new AddIdenticalPayloadBasedRecipes(CancellationToken)
            {
                Logger = Logger,
                Diff = diff,
                SourceTokens = SourceTokens,
                TargetTokens = TargetTokens,
                UsePayloadDevicePaths = UsePayloadDevicePaths,
                UseSourceFile = UseSourceFile
            };

            worker.Execute();

            TotalBytesCopiedFromIdenticalPayload = worker.TotalBytesCopiedFromIdenticalPayload;
            ChunkCountCopiedFromIdenticalPayload = worker.ChunkCountCopiedFromIdenticalPayload;

            LogChunksMissingRecipe(Logger, diff);
            Logger.LogInformation($"{ChunkCountCopiedFromIdenticalPayload} chunks ({TotalBytesCopiedFromIdenticalPayload:#,0.####} bytes) copied from source archive chunks due to identical payload.");
        }

        void AddDeltaBasedRecipes()
        {
            var worker = new AddDeltaBasedRecipes(CancellationToken)
            {
                Logger = Logger,
                Diff = diff,
                SourceTokens = SourceTokens,
                TargetTokens = TargetTokens,
                DeltaTokens = DeltaTokens,
                UsePayloadDevicePaths = UsePayloadDevicePaths,
                UseSourceFile = UseSourceFile
            };

            worker.Execute();

            TotalBytesFromDeltaBasedRecipes = worker.TotalBytesFromDeltaBasedRecipes;
            ChunkCountFromDeltaBasedRecipes = worker.ChunkCountFromDeltaBasedRecipes;

            LogChunksMissingRecipe(Logger, diff);
            Logger.LogInformation($"{ChunkCountFromDeltaBasedRecipes} chunks ({TotalBytesFromDeltaBasedRecipes:#,0.####} bytes) based off of deltas.");
        }

        void AddRemainderChunks()
        {
            var worker = new AddRemainderChunks(CancellationToken)
            {
                Logger = Logger,
                WorkingFolder = WorkingFolder,
                TargetFile = TargetFile,
                Diff = diff
            };

            worker.Execute();

            LogChunksMissingRecipe(Logger, diff);
        }

        void FinalizeInlineAssets()
        {
            var worker = new FinalizeInlineAssets(CancellationToken) { Logger = Logger, Diff = diff };
            worker.Execute();
        }

        void DumpDiff()
        {
            var worker = new DumpDiff(CancellationToken) { Logger = Logger, LogFolder = LogFolder, Diff = diff };
            worker.Execute();
        }

        void WriteDiffToDisk()
        {
            var worker = new WriteDiffToDisk(CancellationToken) { Logger = Logger, OutputFile = OutputFile, Diff = diff };
            worker.Execute();
        }
        #endregion

        private void WriteToLogFile(string content, string logFileName)
        {
            using (var stream = File.OpenWrite(Path.Combine(LogFolder, logFileName)))
            {
                using (var writer = new StreamWriter(stream, Encoding.UTF8, -1, true))
                {
                    writer.Write(content);
                }
            }
        }

        static int ChunkWithNoRecipeCount(Diff diff) => diff.Chunks.Count(c => c.Recipes.Count == 0);

        static void LogChunksMissingRecipe(ILogger logger, Diff diff)
        {
            logger.LogInformation($"{ChunkWithNoRecipeCount(diff)} chunks still need a recipe.");
        }

        protected override void ExecuteInternal()
        {
            try
            {
                ExecuteTasks();
            }
            catch (Exception e)
            {
                Logger.LogException(e);
                throw;
            }
        }

        /// <summary>
        /// The general idea here is to build a new diff with all of the chunks from the target
        /// archive to represent the structure. The new diff should have no recipes from the original
        /// target, because the goal here is to create the diff without having access to the
        /// target payload - the original recipes for chunks in the target will all refer to
        /// payload entries in the target, which will not be on a machine when we try to
        /// hydrate using a diff.
        /// Instead, we look at the source archive in several different ways to try to figure out
        /// how to reconstruct the target chunks and add recipes as we find them.
        /// We will use direct copies of chunks as well as transforming data with deltas to do this.
        /// Lastly, for any chunks where we can't determine a recipe we will throw them into the
        /// "remainder". The remainder is a large blob contained in the diff that will be used to
        /// generate any file that we can't find any other means to regenerate. The blob itself
        /// is compressed using zlib to minimize the size impact.
        /// </summary>
        protected void ExecuteTasks()
        {
            List<Action> diffBuildingActions = new()
            {
                () =>
                {
                    LogParameters();
                    EnsureWorkingFolder();
                },

                // Decompress image files in source/target archives and recompress them with zstd_compress_file.
                Recompress,

                // Create archive tokenization for target and source and expand payload for both.
                Expand,

                // Create a diff with all of the target chunks, but no recipes (except for all-zero chunks).
                CreateDiff,

                // Add copies of existing targets, when we have the same hash occuring multiple times
                () => { if (UseCopyTargetRecipes) { AddCopiesForDuplicateChunks(); } },

                // Add recipes for content that is based off of payload we can find in the source
                AddIdenticalPayloadBasedRecipes,

                // Add copies of chunks from the source archive that match exactly with hashes in the target.
                AddSourceCopyRecipes,

                // Create deltas between source and target payload using the diff to decide if we a individual delta
                // would be helpful. We don't create a delta if it's not going to be used in a chunk's recipe.
                CreateDeltas,

                // Add recipes chunks using deltas we generated
                AddDeltaBasedRecipes,

                // Create a basic recipe using the raw bits of the chunk and put that data into the "remainder"
                AddRemainderChunks,

                //            // Alter second occurrence of a given inline asset so we don't store it twice
                //            FinalizeInlineAssets,

                // Write a JSON file for the diff
                DumpDiff,

                // Write the diff as a binary file
                WriteDiffToDisk,
            };

            foreach(var action in diffBuildingActions)
            {
                CheckForCancellation();
                action();
            }

            if (!KeepWorkingFolder)
            {
                Logger.LogInformation($"Deleting WorkingFolder: {WorkingFolder}");
                Directory.Delete(WorkingFolder);
            }
        }

        public static string GetPayloadRootPath(string workingFolder, string archiveSubFolder)
        {
            return Path.Combine(workingFolder, archiveSubFolder, "payload");
        }

        public static string GetPayloadPath(string workingFolder, string archiveSubFolder, ArchiveItem payload)
        {
            return Path.Combine(GetPayloadRootPath(workingFolder, archiveSubFolder), payload.ExtractedPath);
        }

        public static string GetSourcePayloadPath(string workingFolder, ArchiveItem payload)
        {
            return GetPayloadPath(workingFolder, "source", payload);
        }

        public static string GetTargetPayloadPath(string workingFolder, ArchiveItem payload)
        {
            return GetPayloadPath(workingFolder, "target", payload);
        }

        public static string GetChunksRootPath(string workingFolder, string archiveSubFolder)
        {
            return Path.Combine(workingFolder, archiveSubFolder, "chunks");
        }

        public static string GetChunkPath(string workingFolder, string archiveSubFolder, ArchiveItem chunk)
        {
            return Path.Combine(GetChunksRootPath(workingFolder, archiveSubFolder), chunk.Id.ToString());
        }
        public static string GetSourceChunkPath(string workingFolder, ArchiveItem chunk)
        {
            return GetChunkPath(workingFolder, "source", chunk);
        }

        public static string GetTargetChunkPath(string workingFolder, ArchiveItem chunk)
        {
            return GetChunkPath(workingFolder, "target", chunk);
        }

        public static string GetTargetArchiveItemPath(string workingFolder, ArchiveItem item)
        {
            switch (item.Type)
            {
                case ArchiveItemType.Chunk: return GetTargetChunkPath(workingFolder, item);
                case ArchiveItemType.Payload: return GetTargetPayloadPath(workingFolder, item);
                default: throw new Exception($"Unexpected archive item type: {item.Type}");
            }
        }

        public static string GetSourceArchiveItemPath(string workingFolder, ArchiveItem item)
        {
            switch (item.Type)
            {
                case ArchiveItemType.Chunk: return GetSourceChunkPath(workingFolder, item);
                case ArchiveItemType.Payload: return GetSourcePayloadPath(workingFolder, item);
                default: throw new Exception($"Unexpected archive item type: {item.Type}");
            }
        }

        public static string GetDeltasPath(string workingFolder)
        {
            return Path.Combine(workingFolder, "deltas");
        }

        public static string GetDeltaPath(string workingFolder, ArchiveItem sourcePayload, ArchiveItem targetPayload)
        {
            return Path.Combine(GetDeltasPath(workingFolder), $"{sourcePayload.Id}.{targetPayload.Id}");
        }

        public static string GetArchiveFolder(string workingFolder, string archiveSubFolder)
        {
            return Path.Combine(workingFolder, archiveSubFolder);
        }

        public static string GetArchiveJson(string workingFolder, string archiveSubFolder)
        {
            return Path.Combine(GetArchiveFolder(workingFolder, archiveSubFolder), "archive.json");
        }

        public static string GetDeltaTokensPath(string workingFolder)
        {
            return Path.Combine(workingFolder, "DeltaTokens.json");
        }
    }
}
