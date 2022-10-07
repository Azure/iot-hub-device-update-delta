/**
 * @file Program.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Azure.DeviceUpdate.Diffs;

using System.Runtime.InteropServices;
using ArchiveUtility;

namespace DiffGenTool
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Platform: {0}", RuntimeInformation.OSDescription);
            Console.WriteLine("Running Directory: {0}", ProcessHelper.RunningDirectory);

            if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                if (args.Length < 5 || args.Length > 7)
                {
                    Console.WriteLine("Usage: DiffGenTool <source archive> <target archive> <output path> <log folder> <working folder>");
                    Console.WriteLine("       DiffGenTool <source archive> <target archive> <output path> <log folder> <working folder> <recompressed target archive>");
                    Console.WriteLine("       DiffGenTool <source archive> <target archive> <output path> <log folder> <working folder> <recompressed target archive> \"<signing command>\"");
                    Environment.Exit(1);
                }
            }
            else
            {
                if (args.Length != 5)
                {
                    Console.WriteLine("Usage: DiffGenTool <source archive> <target archive> <output path> <log folder> <working folder>");
                    Environment.Exit(1);
                }
            }

            string sourceFile = args[0];
            string targetFile = args[1];
            string outputFile = args[2];
            string logFolder = args[3];
            string workingFolder = args[4];
            string recompressedTargetFile = args.Length >= 6 ? args[5] : string.Empty;
            string signingCommand = args.Length == 7 ? args[6] : string.Empty;

            CancellationTokenSource cancellationTokenSource = new();
            DiffBuilder.Parameters parameters = new()
            {
                SourceFile = sourceFile,
                TargetFile = targetFile,
                OutputFile = outputFile,
                LogFolder = logFolder,
                WorkingFolder = workingFolder,
                RecompressedTargetFile = recompressedTargetFile,
                SigningCommand = signingCommand,
                KeepWorkingFolder = true,
                CancellationToken = cancellationTokenSource.Token
            };

            DiffBuilder.ParametersStatus parametersStatus = parameters.Validate(out string[] issues);

            if (parametersStatus != DiffBuilder.ParametersStatus.Ok)
            {
                Console.WriteLine($"Parameters failed. Status: {parametersStatus}");
                Console.WriteLine("Issues:");
                foreach(var issue in issues)
                {
                    Console.WriteLine(issue);
                }

                return;
            }

            Task task = DiffBuilder.ExecuteAsync(parameters);

            try
            {
                task.Wait();
            }
            catch (AggregateException ae)
            {
                ae.Handle(e =>
                {
                    if (e is TaskCanceledException)
                    {
                        Console.WriteLine("Diff generation has been canceled by the user.");
                        if (Directory.Exists(workingFolder))
                        {
                            Console.WriteLine("Working folder had already been created. Manual cleanup may be required.");
                            Console.WriteLine($"Working folder path: {workingFolder}");
                        }
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                });
            }
        }
    }
}
