/**
 * @file LoggerExtension.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.Extensions.Logging;

    public static class LoggerExtension
    {
        public static void LogException(this ILogger logger, Exception e)
        {
            logger.LogException(e, LogLevel.Error);
        }

        private static string currentProcessName = Process.GetCurrentProcess().ProcessName;

        private static void AppendLogLine(StringBuilder sb, LogLevel level, string line)
        {
            sb.Append($"{currentProcessName} : {LogLevelText(null, level)} : {line}{Environment.NewLine}");
        }

        public static void LogException(this ILogger logger, Exception e, LogLevel level)
        {
            if (e.InnerException != null)
            {
                logger.LogException(e.InnerException, level);
            }

            var message = new StringBuilder();

            StackTrace st = new(e, true);
            if (st.FrameCount > 0)
            {
                StackFrame frame = st.GetFrame(0);
                if (frame != null)
                {
                    message.Append($"{frame.GetFileName()}({frame.GetFileLineNumber()},{frame.GetFileColumnNumber()}):{Environment.NewLine}");
                }
            }

            AppendLogLine(message, level, $"0x{Marshal.GetHRForException(e).ToString("X")}");
            AppendLogLine(message, level, $"EXCEPTION: {e.Message}");

            if (e.StackTrace != null)
            {
                var logLevelText = LogLevelText(logger, level);
                var replaceText = $"{Environment.NewLine}{currentProcessName} : {logLevelText} : ";
                var line = e.StackTrace.Replace(Environment.NewLine, replaceText, StringComparison.OrdinalIgnoreCase);
                AppendLogLine(message, level, line);
            }
            else
            {

                AppendLogLine(message, level, "STACKTRACE: The stack trace was null for this exception.");
            }

            logger.Log(level, message.ToString());
        }

        private static Dictionary<LogLevel, string> logLevelText = new()
        {
            { LogLevel.Information, "Info" },
        };

        public static string LogLevelText(this ILogger logger, LogLevel logLevel)
        {
            if (!logLevelText.ContainsKey(logLevel))
            {
                return logLevel.ToString();
            }

            return logLevelText[logLevel];
        }

    }
}
