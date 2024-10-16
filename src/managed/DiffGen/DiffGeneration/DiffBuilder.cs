/**
* @file DiffBuilder.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs;

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using ArchiveUtility;
using Ext4Archives;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Azure.DeviceUpdate.Diffs.Workers;
using Microsoft.Extensions.Logging;

[SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
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

        public ArchiveTokenization TargetTokens { get; set; }

        public string OutputFile { get; set; }

        public string LogFolder { get; set; }

        public string WorkingFolder { get; set; }

        public string RecompressedTargetFile { get; set; }

        public string SigningCommand { get; set; }

        public bool KeepWorkingFolder { get; set; } = false;

        public bool UseCopyTargetRecipes { get; set; } = true;

        public CancellationToken CancellationToken { get; set; } = CancellationToken.None;

        private static bool IsBinaryMissingLinux(string name)
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
        private static extern bool PathFindOnPath([In, Out] StringBuilder pszFile, [In] String[] ppszOtherDirs);

        private static bool IsBinaryMissingWindows(string name)
        {
            const int MAX_PATH = 260;

            string pathInRunningFolder = ProcessHelper.GetPathInRunningDirectory(name);

            if (File.Exists(pathInRunningFolder))
            {
                return false;
            }

            return !PathFindOnPath(new StringBuilder(name, MAX_PATH), null);
        }

        private static bool IsBinaryMissing(string name)
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
                Ext4Archive.DumpextfsPath,
                ZstdDeltaBuilder.ZstdCompressExeFilePath,
                BsDiffDeltaBuilder.BsdiffExePath,
            };

            if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                var missingBinaries = requiredBinaries.Where(b => IsBinaryMissing(b)).ToList();
                if (!DiffApi.CanOpenSession())
                {
                    missingBinaries.Add(DiffApi.AduDiffApiDll);
                }

                return missingBinaries.ToArray();
            }
            else
            {
                requiredBinaries.Add(DiffApi.AduDiffApiDllWindows);
                requiredBinaries.AddRange(DiffApi.AduDiffApiWindowsDependencies);
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
            return new List<string> { SourceFile, TargetFile }
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

        string processReportFile = Path.Combine(parameters.LogFolder, "ProcessesReport.json");

        if (File.Exists(processReportFile))
        {
            File.Delete(processReportFile);
        }

        ProcessHelper.SetProcessesReportFilePath(processReportFile);

        string logPath = Path.Combine(parameters.LogFolder, "DiffBuilder.log");

        using var logger = new DiffLogger(logPath);

        DiffBuilder builder = new(logger, parameters.WorkingFolder, parameters.CancellationToken)
        {
            SourceFile = parameters.SourceFile,
            TargetFile = parameters.TargetFile,
            TargetTokens = parameters.TargetTokens,
            OutputFile = parameters.OutputFile,
            LogFolder = parameters.LogFolder,
            RecompressedTargetFile = parameters.RecompressedTargetFile,
            SigningCommand = parameters.SigningCommand,
            KeepWorkingFolder = parameters.KeepWorkingFolder,
            UseCopyTargetRecipes = parameters.UseCopyTargetRecipes,
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

    public bool UseSourceFile { get; set; } = true;

    public bool UseCopyTargetRecipes { get; set; } = true;
    #endregion

    #region("Internal Properties")
    private Diff diff = null;
    #endregion

    #region("Output Properties")
    public ArchiveTokenization SourceTokens { get; private set; }

    public ArchiveTokenization TargetTokens { get; private set; }

    public DeltaCatalog DeltaCatalog { get; private set; }

    public ulong TotalBytesCopiedFromTarget { get; private set; }

    public int ChunkCountCopiedFromTarget { get; private set; }

    public ulong TotalBytesCopiedFromSourceChunks { get; private set; }

    public int ChunkCountCopiedFromSourceChunks { get; private set; }

    public ulong TotalBytesCopiedFromIdenticalPayload { get; private set; }

    public int ChunkCountCopiedFromIdenticalPayload { get; private set; }

    public ulong TotalBytesFromDeltaBasedRecipes { get; private set; }

    public int ChunkCountFromDeltaBasedRecipes { get; private set; }

    public DeltaPlans DeltaPlans { get; private set; }

    public IEnumerable<ItemDefinition> SourceItemsNeeded { get; private set; }

    public IEnumerable<ItemDefinition> TargetItemsNeeded { get; private set; }
    #endregion

    public DiffBuilder(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    #region("Parameter Methods")
    private void LogParameters()
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
    private void Recompress()
    {
        if (string.IsNullOrEmpty(RecompressedTargetFile))
        {
            return;
        }

        Recompress worker = new(Logger, WorkingFolder, CancellationToken)
        {
            OriginalFile = TargetFile,
            RecompressedFile = RecompressedTargetFile,
            SigningCommand = SigningCommand,
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

    private void TokenizeArchives()
    {
        TokenizeArchives worker = new(Logger, WorkingFolder, CancellationToken)
        {
            SourceFile = SourceFile,
            TargetFile = TargetFile,
            TargetTokens = TargetTokens,
        };
        worker.Execute();

        SourceTokens = worker.SourceTokens;
        TargetTokens = worker.TargetTokens;
    }

    private void CreateDeltas()
    {
        var worker = new CreateDeltas(Logger, WorkingFolder, CancellationToken)
        {
            LogFolder = LogFolder,
            SourceTokens = SourceTokens,
            TargetTokens = TargetTokens,
            UseSourceFile = UseSourceFile,
            DeltaPlans = DeltaPlans,
            KeepWorkingFolder = KeepWorkingFolder,
            Diff = diff,
        };

        worker.Execute();

        DeltaCatalog = worker.DeltaCatalog;
    }

    private void AddRecipesToDiff()
    {
        var worker = new AddRecipesToDiff(Logger, WorkingFolder, CancellationToken)
        {
            SourceTokens = SourceTokens,
            TargetTokens = TargetTokens,
            DeltaCatalog = DeltaCatalog,
            Diff = diff,
        };

        worker.Execute();
    }

    private void CreateDiff()
    {
        diff = new(SourceTokens, TargetTokens);

        diff.Version = 1;
    }

    private void UseRecipesFromSource()
    {
        var worker = new UseRecipesFromSource(Logger, WorkingFolder, CancellationToken)
        {
            SourceTokens = SourceTokens,
            TargetTokens = TargetTokens,
            Diff = diff,
        };

        Logger.LogInformation("Before UseRecipesFromSource: diff.Tokens has {0} recipes.", diff.Tokens.Recipes.Count);
        worker.Execute();
        Logger.LogInformation("After UseRecipesFromSource: diff.Tokens has {0} recipes.", diff.Tokens.Recipes.Count);
    }

    private void SelectItemsForDelta()
    {
        var worker = new SelectItemsForDelta(Logger, WorkingFolder, CancellationToken)
        {
            SourceFile = SourceFile,
            TargetFile = TargetFile,
            SourceTokens = SourceTokens,
            TargetTokens = TargetTokens,
            Diff = diff,
        };

        worker.Execute();

        SourceItemsNeeded = worker.SourceItemsNeeded;
        TargetItemsNeeded = worker.TargetItemsNeeded;
        DeltaPlans = worker.DeltaPlans;
    }

    private void ExtractItemsForDelta()
    {
        SourceTokens.WorkingFolder = Path.Combine(WorkingFolder, "source");
        TargetTokens.WorkingFolder = Path.Combine(WorkingFolder, "target");

        var worker = new ExtractItemsForDelta(Logger, WorkingFolder, CancellationToken)
        {
            SourceFile = SourceFile,
            TargetFile = TargetFile,
            SourceTokens = SourceTokens,
            TargetTokens = TargetTokens,
            SourceItemsNeeded = SourceItemsNeeded,
            TargetItemsNeeded = TargetItemsNeeded,
        };

        worker.Execute();
    }

    private void CreateInlineAssets()
    {
        var worker = new CreateInlineAssets(Logger, WorkingFolder, CancellationToken)
        {
            Diff = diff,
            DeltaCatalog = DeltaCatalog,
        };

        worker.Execute();
    }

    private void AddRemainderChunks()
    {
        var worker = new AddRemainderChunks(Logger, WorkingFolder, CancellationToken)
        {
            TargetFile = TargetFile,
            TargetTokens = TargetTokens,
            Diff = diff,
        };

        worker.Execute();
    }

    private void DumpDiff()
    {
        var worker = new DumpDiff(Logger, WorkingFolder, CancellationToken) { LogFolder = LogFolder, Diff = diff };
        worker.Execute();
    }

    private void WriteDiffToDisk()
    {
        var worker = new WriteDiffToDisk(Logger, WorkingFolder, CancellationToken) { OutputFile = OutputFile, Diff = diff };
        worker.Execute();
    }

    private void VerifyDiff()
    {
        var worker = new VerifyDiff(Logger, WorkingFolder, CancellationToken) { OutputFile = OutputFile, Diff = diff, SourceFile = SourceFile };
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

    protected override void ExecuteInternal()
    {
        ExecuteTasks();
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

            // Create archive tokenization for target and source
            TokenizeArchives,

            // Create a diff with all of the target chunks, but no recipes (except for all-zero chunks).
            CreateDiff,

            // Add recipes for content that is based off of payload we can find in the source
            UseRecipesFromSource,

            // Select items for delta
            SelectItemsForDelta,

            // Extract items for delta
            ExtractItemsForDelta,

            // Create deltas between source and target payload using the diff to decide if we a individual delta
            // would be helpful. We don't create a delta if it's not going to be used in a chunk's recipe.
            CreateDeltas,

            // Create Inline Assets file and recipes for entries
            CreateInlineAssets,

            // Add recipes from deltas or forward recipes from target
            AddRecipesToDiff,

            // Create a basic recipe using the raw bits of the chunk and put that data into the "remainder"
            AddRemainderChunks,

            // Write a JSON file for the diff
            DumpDiff,

            // Write the diff as a binary file
            WriteDiffToDisk,

            // Verify the diff contents
            VerifyDiff,
        };

        foreach (var action in diffBuildingActions)
        {
            CheckForCancellation();
            action();
        }

        if (!KeepWorkingFolder)
        {
            Logger.LogInformation($"Deleting WorkingFolder: {WorkingFolder}");
            Directory.Delete(WorkingFolder);
        }

        SummarizeResults(Logger, SourceFile, TargetFile, OutputFile);
    }

    public static string GetDeltaRoot(string workingFolder)
    {
        return Path.Combine(workingFolder, "deltas");
    }

    public static string GetDeltasPath(string workingFolder)
    {
        return Path.Combine(workingFolder, "deltas");
    }

    public static string GetDeltaCatalogPath(string workingFolder)
    {
        return Path.Combine(workingFolder, "DeltaCatalog.json");
    }

    private static void SummarizeResults(ILogger logger, string sourceFile, string targetFile, string diffFile)
    {
        var sourceSize = new FileInfo(sourceFile).Length;
        logger.LogInformation("Source File: {sourceFile} - Size {sourceSize:#,0.####}", sourceFile, sourceSize);

        var targetSize = new FileInfo(targetFile).Length;
        logger.LogInformation("Target File: {targetFile} - Size {targetSize:#,0.####}", targetFile, targetSize);

        var diffSize = new FileInfo(diffFile).Length;
        logger.LogInformation("Diff File  : {diffFile} - Size {diffSize:#,0.####}", diffFile, diffSize);

        if (diffSize > targetSize)
        {
            throw new DiffBuilderException($"Diff ({diffSize:#,0.####}) is larger than target archive ({targetSize:#,0.####})!", FailureType.DiffGeneration);
        }

        var percentSize = 100.0 * (((float)targetSize - diffSize) / targetSize);

        logger.LogInformation("Diff is {percentSize:#,0.##}% reduction in size from target.", percentSize);
    }
}
