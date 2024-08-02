namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Threading;

    using Microsoft.Extensions.Logging;

    public class ArchiveLoaderContext
    {
        public static ILogger DefaultLogger { get; set; }

        public static LogLevel DefaultLogLevel { get; set; } = LogLevel.Information;

        public Stream Stream { get; }

        public string WorkingFolder { get; }

        public Dictionary<Type, ArchiveTokenization> TokenCache { get; } = new();

        public ItemDefinition ArchiveItem { get; set; } = null;

        public ILogger Logger { get; }

        public LogLevel LogLevel { get; }

        public CancellationToken CancellationToken { get; set; } = CancellationToken.None;

        public string OriginalArchiveFileName { get; set; } = string.Empty;

        public ArchiveUseCase UseCase { get; set; } = ArchiveUseCase.Generic;

        public ArchiveLoaderContext(Stream stream, string workingFolder, ILogger logger, LogLevel logLevel)
        {
            Stream = stream;
            WorkingFolder = workingFolder;
            Logger = logger;
            LogLevel = logLevel;
        }

        public ArchiveLoaderContext(Stream stream, string workingFolder)
        {
            Stream = stream;
            WorkingFolder = workingFolder;
            Logger = DefaultLogger;
            LogLevel = DefaultLogLevel;
        }
    }
}
