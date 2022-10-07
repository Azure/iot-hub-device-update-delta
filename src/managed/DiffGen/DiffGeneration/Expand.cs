/**
 * @file Expand.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Threading.Tasks;

    using ArchiveUtility;
    using CpioArchives;
    using TarArchives;
    using SWUpdateArchives;
    using Ext4Archives;
    using System.Threading;
    using Microsoft.Extensions.Logging;

    class Expand : Worker
    {
        public string SourceTokensFile { get; set; }
        public string TargetTokensFile { get; set; }
        public string SourceFile { get; set; }
        public string TargetFile { get; set; }
        public ArchiveTokenization SourceTokens { get; set; }
        public ArchiveTokenization TargetTokens { get; set; }

        static Expand()
        {
            ArchiveLoader.RegisterArchiveType(typeof(OldStyleTarArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(AsciiCpioArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(NewAsciiCpioArchive), 99);
            ArchiveLoader.RegisterArchiveType(typeof(BinaryCpioArchive), 100);
            ArchiveLoader.RegisterArchiveType(typeof(SWUpdateArchive), 98);
            ArchiveLoader.RegisterArchiveType(typeof(Ext4Archive), 100);
        }

        public Expand(CancellationToken cancellationToken) :
            base(cancellationToken)
        {
        }

        protected override void ExecuteInternal()
        {
            ExpandArchive expandSource = new ExpandArchive(CancellationToken)
            {
                Logger = Logger, WorkingFolder = WorkingFolder,
                ArchivePath = SourceFile, ArchiveSubFolder = "source",
                ExposeCompressedContent = ArchiveTokenization.ExposeCompressedPayloadType.All,
                PackageFailureType = FailureType.SourcePackageInvalid,
            };
            ExpandArchive expandTarget = new ExpandArchive(CancellationToken)
            {
                Logger = Logger, WorkingFolder = WorkingFolder,
                ArchivePath = TargetFile, ArchiveSubFolder = "target",
                ExposeCompressedContent = ArchiveTokenization.ExposeCompressedPayloadType.ValidatedZstandard,
                PackageFailureType = FailureType.TargetPackageInvalid,
            };

            CheckForCancellation();
            ParallelOptions po = new() { CancellationToken = CancellationToken };
            Parallel.ForEach(new List<Worker> { expandSource, expandTarget }, po, worker =>
            {
                worker.Execute();
            });

            SourceTokensFile = DiffBuilder.GetArchiveJson(WorkingFolder, "source");
            TargetTokensFile = DiffBuilder.GetArchiveJson(WorkingFolder, "target");
            SourceTokens = expandSource.Tokens;
            TargetTokens = expandTarget.Tokens;
        }

        class ExpandArchive : Worker
        {
            public string ArchivePath { get; set; }
            public FailureType PackageFailureType { get; set; }
            public DiffBuilder DiffBuilder { get; set; }
            public string ArchiveSubFolder { get; set; }
            public ArchiveTokenization.ExposeCompressedPayloadType ExposeCompressedContent { get; set; } = ArchiveTokenization.ExposeCompressedPayloadType.None;
            public ArchiveTokenization Tokens { get; set; }

            public ExpandArchive(CancellationToken cancellationToken) :
                base(cancellationToken)
            {
            }

            void LoadTokens(Stream stream, string jsonPath, out ArchiveTokenization tokens)
            {
                string cookiePath = jsonPath + ".cookie";
                if (File.Exists(cookiePath))
                {
                    Logger.LogInformation($"Found cookie for expansion: {cookiePath}");
                    Logger.LogInformation($"Loading previous JSON from: {jsonPath}");

                    try
                    {
                        tokens = ArchiveTokenization.FromJsonPath(jsonPath);
                        return;
                    }
                    catch (Exception e)
                    {
                        Logger.LogException(e);
                        Logger.LogError("Found previous cookie, but failed to load archive tokenization JSON.");
                    }
                }

                Logger.LogInformation($"Loading Archive into tokens. JsonPath: {jsonPath}");
                ArchiveLoaderContext archiveLoaderContext = new(stream, WorkingFolder)
                {
                    Logger = Logger,
                    CancellationToken = CancellationToken
                };
                ArchiveLoader.LoadArchive(archiveLoaderContext, out tokens);

                Logger.LogInformation($"ExposeCompressedContent: {ExposeCompressedContent}");

                tokens.ExposeCompressedPayload(stream, ExposeCompressedContent);

                Logger.LogInformation($"Writing Json.");
                using (var jsonStream = File.OpenWrite(jsonPath))
                {
                    tokens.WriteJson(jsonStream, true);
                }

                CreateCookie(cookiePath);
            }

            protected override void ExecuteInternal()
            {
                string folder = DiffBuilder.GetArchiveFolder(WorkingFolder, ArchiveSubFolder);
                Logger.LogInformation("Expanding {0} to {1}", ArchivePath, folder);

                Directory.CreateDirectory(folder);

                ArchiveTokenization tokens;

                const string ARCHIVE_HASH_COOKIE = "ArchiveHash.Cookie";

                var hashOfArchive = Hash.FromFile(ArchivePath).ToString();

                string archiveHashCookiePath = Path.Combine(folder, ARCHIVE_HASH_COOKIE);

                if (File.Exists(archiveHashCookiePath))
                {
                    var oldHashOfArchive = File.ReadAllText(archiveHashCookiePath);

                    if (!hashOfArchive.Equals(oldHashOfArchive))
                    {
                        var message = $"Archive working folder {folder} was previously used with an archive of "
                            + $"hash {oldHashOfArchive}, but new archive, {ArchivePath}, has a hash of {hashOfArchive}";
                        Logger.LogError(message);
                        throw new DiffBuilderException(message, PackageFailureType);
                    }
                }
                else
                {
                    File.WriteAllText(archiveHashCookiePath, hashOfArchive);
                }

                using (var stream = File.OpenRead(ArchivePath))
                {
                    CheckForCancellation();

                    var jsonPath = DiffBuilder.GetArchiveJson(WorkingFolder, ArchiveSubFolder);

                    Stopwatch stopWatch = new();
                    stopWatch.Start();
                    Logger.LogInformation($"Loading tokens from {ArchivePath}");
                    LoadTokens(stream, jsonPath, out tokens);
                    stopWatch.Stop();
                    Logger.LogInformation($"Finished loading tokens. Time: {stopWatch.Elapsed}");

                    Tokens = tokens;

                    var cookiePath = folder + ".cookie";
                    if (File.Exists(cookiePath))
                    {
                        Logger.LogInformation($"Found cookie for expansion. Using previous run result. Cookie: {cookiePath}");
                        return;
                    }

                    CheckForCancellation();
                    var payloadFolder = DiffBuilder.GetPayloadRootPath(WorkingFolder, ArchiveSubFolder);
                    Logger.LogInformation("Extracting payload to: {0}", payloadFolder);
                    stopWatch.Reset();
                    stopWatch.Start();
                    PayloadExtractor.ExtractPayload(tokens, stream, payloadFolder);
                    CheckForCancellation();
                    PayloadExtractor.ValidateExtractedPayload(tokens, payloadFolder);
                    stopWatch.Stop();
                    Logger.LogInformation($"Finished extracting {ArchiveSubFolder} payload. Time: {stopWatch.Elapsed}");

                    DiffBuilder.CreateCookie(cookiePath);
                }
            }
        }
    }
}
