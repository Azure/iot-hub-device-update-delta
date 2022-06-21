/**
 * @file ProcessHelper.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace ArchiveUtility
{
    public static class ProcessHelper
    {
        class ProcessEntry
        {
            public string ProcessName { get; set; }
            public string StartFileName { get; set; }
            public string StartArguments { get; set; }
            public int Id { get; set; }
            public DateTime StartTime { get; set; }
        }

        class ProcessesReport
        {
            public List<ProcessEntry> Processes { get; set; } = new();
        }

        const int ONE_SECOND = 1000;

        private static string ProcessesReportFilePath = null;

        private static ProcessesReport CurrentProcessesReport = new();

        public static bool WaitForExit(this Process process, int timeoutMillis, CancellationToken cancellationToken)
        {
            //Instead of waiting all the way through, we poll for the process state every second,
            //so we can also check for the cancellation token
            while (!process.HasExited && (DateTime.Now - process.StartTime).TotalMilliseconds < timeoutMillis)
            {
                if (cancellationToken != CancellationToken.None && cancellationToken.IsCancellationRequested)
                {
                    process.Kill();
                    cancellationToken.ThrowIfCancellationRequested();
                }
                process.WaitForExit(ONE_SECOND);
            }

            return process.HasExited;
        }

        public static void SetProcessesReportFilePath(string path)
        {
            lock (CurrentProcessesReport)
            {
                ProcessesReportFilePath = path;
            }
        }

        private static void StoreAndReportProcess(ProcessEntry processEntry)
        {
            lock (CurrentProcessesReport)
            {
                CurrentProcessesReport.Processes.Add(processEntry);

                if (ProcessesReportFilePath != null)
                {
                    try
                    {
                        JsonHelper.Serialize(CurrentProcessesReport, ProcessesReportFilePath);
                    }
                    catch (Exception) { }
                }
            }
        }

        public static bool StartAndReport(this Process process)
        {
            bool ret = process.Start();

            //collect data about the process
            try
            {
                ProcessEntry processEntry = new()
                {
                    ProcessName = process.ProcessName,
                    StartFileName = process.StartInfo.FileName,
                    StartArguments = process.StartInfo.Arguments,
                    Id = process.Id,
                    StartTime = process.StartTime,
                };

                //store this data and then write the report file
                Task.Run(() => StoreAndReportProcess(processEntry));
            }
            catch (InvalidOperationException) {}
            catch (Win32Exception) {}
            
            return ret;
        }

        public static string RunningDirectory { get => Path.GetDirectoryName(typeof(ProcessHelper).Assembly.Location); }

        public static string GetPathInRunningDirectory(string name)
        {
            return Path.Combine(RunningDirectory, name);
        }

        public static bool CanReadFile(string path)
        {
            try
            {
                using var stream = File.OpenRead(path);
            }
            catch (IOException)
            {
                return false;
            }

            return true;
        }

        public static void WaitForFileAvailable(string path)
        {
            if (CanReadFile(path))
            {
                return;
            }

            const int WAIT_FOR_FILE_AVAILABILITY = 3000; // three seconds
            Thread.Sleep(WAIT_FOR_FILE_AVAILABILITY);
        }
    }
}
