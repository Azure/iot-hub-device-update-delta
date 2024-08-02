/**
* @file TokenizeArchives.cs
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
namespace Microsoft.Azure.DeviceUpdate.Diffs.Workers;

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

using ArchiveUtility;
using CpioArchives;
using Ext4Archives;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;
using Microsoft.Extensions.Logging;
using SWUpdateArchives;
using TarArchives;
using ZipArchives;

public class TokenizeArchives : Worker
{
    public string SourceTokensFile { get; set; }

    public string TargetTokensFile { get; set; }

    public string SourceFile { get; set; }

    public string TargetFile { get; set; }

    public ArchiveTokenization SourceTokens { get; set; }

    public ArchiveTokenization TargetTokens { get; set; }

    static TokenizeArchives()
    {
        ArchiveLoader.RegisterArchiveType(typeof(OldStyleTarArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(AsciiCpioArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(NewAsciiCpioArchive), 99);
        ArchiveLoader.RegisterArchiveType(typeof(BinaryCpioArchive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(SWUpdateArchive), 98);
        ArchiveLoader.RegisterArchiveType(typeof(Ext4Archive), 100);
        ArchiveLoader.RegisterArchiveType(typeof(GzArchive), 100);
    }

    public TokenizeArchives(ILogger logger, string workingFolder, CancellationToken cancellationToken)
        : base(logger, workingFolder, cancellationToken)
    {
    }

    protected override void ExecuteInternal()
    {
        TokenizeArchive tokenizeSource = new TokenizeArchive(Logger, WorkingFolder, CancellationToken)
        {
            ArchivePath = SourceFile,
            ArchiveSubFolder = "source",
            PackageFailureType = FailureType.SourcePackageInvalid,
            UseCase = ArchiveUseCase.DiffSource,
        };

        TokenizeArchive tokenizeTarget = new TokenizeArchive(Logger, WorkingFolder, CancellationToken)
        {
            ArchivePath = TargetFile,
            Tokens = TargetTokens,
            ArchiveSubFolder = "target",
            PackageFailureType = FailureType.TargetPackageInvalid,
            UseCase = ArchiveUseCase.DiffTarget,
        };

        var workers = new List<Worker> { tokenizeSource, tokenizeTarget };

        CheckForCancellation();
        ParallelOptions po = new() { CancellationToken = CancellationToken };
        Parallel.ForEach(workers, po, worker =>
        {
            worker.Execute();
        });

        var sourceFolder = Path.Combine(WorkingFolder, "source");
        SourceTokensFile = GetArchiveJson(sourceFolder);
        var targetFolder = Path.Combine(WorkingFolder, "target");
        TargetTokensFile = GetArchiveJson(targetFolder);
        SourceTokens = tokenizeSource.Tokens;
        TargetTokens = tokenizeTarget.Tokens;
    }

    public class TokenizeArchive : Worker
    {
        public string ArchivePath { get; set; }

        public FailureType PackageFailureType { get; set; }

        public DiffBuilder DiffBuilder { get; set; }

        public string ArchiveSubFolder { get; set; }

        public ArchiveTokenization Tokens { get; set; }

        public ArchiveUseCase UseCase { get; set; }

        public TokenizeArchive(ILogger logger, string workingFolder, CancellationToken cancellationToken)
            : base(logger, workingFolder, cancellationToken)
        {
        }

        private bool TryLoadPreviousTokens(string path, out ArchiveTokenization tokens)
        {
            Logger.LogInformation("Loading previous JSON from: {path}", path);

            string cookiePath = path + ".cookie";
            if (!File.Exists(cookiePath))
            {
                Logger.LogInformation("No file found at: {path}", path);
                tokens = null;
                return false;
            }

            Logger.LogInformation($"Found cookie for expansion: {cookiePath}");

            try
            {
                tokens = ArchiveTokenization.FromJsonPath(path);
                return true;
            }
            catch (Exception e)
            {
                Logger.LogException(e);
                Logger.LogError("Found previous cookie, but failed to load archive tokenization JSON.");
            }

            tokens = null;
            return false;
        }

        private void LoadTokens(ArchiveUseCase useCase, string archiveFolder, out ArchiveTokenization tokens)
        {
            var jsonPath = GetArchiveJson(archiveFolder);
            if (TryLoadPreviousTokens(jsonPath, out tokens))
            {
                return;
            }

            using var stream = File.OpenRead(ArchivePath);

            Logger.LogInformation($"Loading Archive from file: {ArchivePath}");
            ArchiveLoaderContext archiveLoaderContext = new(stream, archiveFolder, Logger, LogLevel.Information)
            {
                CancellationToken = CancellationToken,
                UseCase = useCase,
                OriginalArchiveFileName = Path.GetFileName(ArchivePath),
            };

            if (!ArchiveLoader.TryLoadArchive(archiveLoaderContext, out tokens))
            {
                throw new Exception("Couldn't load any format for archive.json: {ArchivePath}");
            }

            Logger.LogInformation("Writing Json: {jsonPath}", jsonPath);
            using var jsonStream = File.OpenWrite(jsonPath);
            tokens.WriteJson(jsonStream, true);

            string cookiePath = jsonPath + ".cookie";
            CreateCookie(cookiePath);
        }

        protected override void ExecuteInternal()
        {
            string folder = Path.Combine(WorkingFolder, ArchiveSubFolder);
            Logger.LogInformation("Tokenizing {ArchivePath} to {folder}", ArchivePath, folder);

            Directory.CreateDirectory(folder);

            ArchiveTokenization tokens;

            const string ARCHIVE_HASH_COOKIE = "ArchiveHash.Cookie";

            Hash archiveHash;

            string archiveHashCookiePath = Path.Combine(folder, ARCHIVE_HASH_COOKIE);

            if (Tokens != null)
            {
                Logger.LogInformation("Using supplied tokens");
                archiveHash = Tokens.ArchiveItem.GetSha256Hash();
            }
            else
            {
                archiveHash = Hash.FromFile(ArchivePath);
            }

            var archiveHashString = archiveHash.ToString();

            if (File.Exists(archiveHashCookiePath))
            {
                var oldHashOfArchive = File.ReadAllText(archiveHashCookiePath);

                if (!archiveHashString.Equals(oldHashOfArchive))
                {
                    var message = $"Working directory was already used for another target archive. Clean contents or use another directory." + Environment.NewLine;
                    message += $"Hash of original archive: {oldHashOfArchive}. {ArchivePath} has a hash of {archiveHashString}";
                    throw new DiffBuilderException(message, FailureType.BadWorkingFolder);
                }
            }
            else
            {
                File.WriteAllText(archiveHashCookiePath, archiveHashString);
            }

            var jsonPath = GetArchiveJson(folder);
            var cookiePath = jsonPath + ".cookie";

            if (Tokens != null)
            {
                Logger.LogInformation($"Writing Json: {jsonPath}");
                using var jsonStream = File.OpenWrite(jsonPath);
                Tokens.WriteJson(jsonStream, true);
            }
            else
            {
                CheckForCancellation();

                Stopwatch stopWatch = new();
                stopWatch.Start();
                Logger.LogInformation("Loading tokens from {ArchivePath}", ArchivePath);
                LoadTokens(UseCase, folder, out tokens);
                stopWatch.Stop();
                Logger.LogInformation("Finished loading tokens. Time: {elapsedTime}", stopWatch.Elapsed);

                Tokens = tokens;
            }

            CreateCookie(cookiePath);
        }
    }

    private static string GetArchiveJson(string archiveFolder)
    {
        return Path.Combine(archiveFolder, "archive.json");
    }
}
