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
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;

    using Microsoft.Extensions.Logging;

    public class DiffLogger : ILogger
    {
        class Writer
        {
            private StreamWriter writer;
            private bool consoleLogging;

            public Writer(string path) : this(path, true)
            {
            }

            private Writer(string path, bool consoleLogging)
            {
                this.consoleLogging = consoleLogging;

                writer = File.AppendText(path);
            }

            public void WriteLine(string msg)
            {
                Task task = new Task(() =>
                {
                    lock (writer)
                    {
                        writer.WriteLine(msg);
                        writer.Flush();
                    }

                    if (consoleLogging)
                    {
                        Console.WriteLine(msg);
                    }
                });

                task.Start();
            }
        }

        internal class NativeMethods
        {

            [DllImport("libc", EntryPoint = "gettid")]
            public static extern uint gettid();

            [DllImport("kernel32.dll")]
            public static extern uint GetCurrentThreadId();

            public static uint GetCurrentThreadIdWrapper()
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    return gettid();
                }

                return GetCurrentThreadId();
            }
        }

        public string LogPath { get; private set; }
        private Writer writer;
        public DiffLogger(string path)
        {
            LogPath = path;
            writer = new Writer(LogPath);
        }
        public IDisposable BeginScope<TState>(TState state) => default;
        public bool IsEnabled(LogLevel logLevel)
        {
            return true;
        }

        public void Log<TState>(LogLevel logLevel, EventId eventId, TState state, Exception exception, Func<TState, Exception, string> formatter)
        {
            if (!IsEnabled(logLevel))
            {
                return;
            }

            writer.WriteLine($"[{this.LogLevelText(logLevel)}] ThreadId{NativeMethods.GetCurrentThreadIdWrapper()} {formatter(state, exception)}");
        }
    }
}
