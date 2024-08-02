/**
 * @file DiffLogger.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.Extensions.Logging;

    public class DiffLogger : ILogger, IDisposable
    {
        public class Writer
        {
            private StreamWriter writer;
            private bool consoleLogging;

            public Writer(string path)
                : this(path, true)
            {
            }

            private Writer(string path, bool consoleLogging)
            {
                this.consoleLogging = consoleLogging;
                this.writer = File.AppendText(path);
            }

            public void WriteLine(string msg)
            {
                Task task = new Task(() =>
                {
                    lock (this.writer)
                    {
                        try
                        {
                            this.writer.WriteLine(msg);
                            this.writer.Flush();
                        }
                        catch (System.ObjectDisposedException)
                        {
                        }
                    }

                    if (this.consoleLogging)
                    {
                        Console.WriteLine(msg);
                    }
                });

                task.Start();
            }

            public void Close()
            {
                writer.Close();
            }
        }

        public string LogPath { get; private set; }

        private Writer writer;

        public DiffLogger(string path)
        {
            this.LogPath = path;
            this.writer = new Writer(LogPath);
        }

        public IDisposable BeginScope<TState>(TState state) => default;

        public bool IsEnabled(LogLevel logLevel)
        {
            return logLevel != LogLevel.None;
        }

        public void Log<TState>(LogLevel logLevel, EventId eventId, TState state, Exception exception, Func<TState, Exception, string> formatter)
        {
            if (!IsEnabled(logLevel))
            {
                return;
            }

            writer.WriteLine($"[{this.LogLevelText(logLevel)}] ThreadId{Thread.CurrentThread.ManagedThreadId} {formatter(state, exception)}");
        }

        public void Dispose()
        {
            writer.Close();
        }
    }
}
