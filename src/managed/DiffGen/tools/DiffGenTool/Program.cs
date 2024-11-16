/**
 * @file Program.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace DiffGenTool;

using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

using ArchiveUtility;

using Microsoft.Azure.DeviceUpdate.Diffs;
using Microsoft.Azure.DeviceUpdate.Diffs.Utility;

public class Program
{
    public static void Main(string[] args)
    {
        Console.WriteLine("Platform: {0}", RuntimeInformation.OSDescription);
        Console.WriteLine("Running Directory: {0}", ProcessHelper.RunningDirectory);

        if (args.Length < 4 || args.Length > 5)
        {
            Console.WriteLine("Usage: DiffGenTool <source archive> <target archive> <output path> <log folder> <working folder>");
            Console.WriteLine("Usage: DiffGenTool <source archive> <target archive> <output path> <working folder>");
            Environment.Exit(1);
        }

        string sourceFile = args[0];
        string targetFile = args[1];
        string outputFile = args[2];

        string workingFolder;
        string logFolder;

        if (args.Length == 4)
        {
            workingFolder = args[3];
            try
            {
                logFolder = MakeLogFolder(workingFolder);
            }
            catch (Exception e)
            {
                WriteErrorMessage(e.Message);
                Environment.Exit(1);
                return;
            }
        }
        else
        {
            logFolder = args[3];
            workingFolder = args[4];
        }

        CancellationTokenSource cancellationTokenSource = new();
        DiffBuilder.Parameters parameters = new()
        {
            SourceFile = sourceFile,
            TargetFile = targetFile,
            OutputFile = outputFile,
            LogFolder = logFolder,
            WorkingFolder = workingFolder,
            KeepWorkingFolder = true,
            CancellationToken = cancellationTokenSource.Token,
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

            Environment.Exit(1);
            return;
        }

        bool caughtException = false;

        try
        {
            Console.WriteLine("Starting main task.");
            DiffBuilder.Execute(parameters);
            Console.WriteLine("Finished main task.");
        }
        catch (AggregateException ae)
        {
            caughtException = true;

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
                else if (e is DiffBuilderException dbe)
                {
                    WriteErrorMessage(dbe.Message);
                    return true;
                }
                else
                {
                    return false;
                }
            });
        }
        catch (TaskCanceledException)
        {
            caughtException = true;

            Console.WriteLine("Diff generation has been canceled by the user.");
            if (Directory.Exists(workingFolder))
            {
                Console.WriteLine("Working folder had already been created. Manual cleanup may be required.");
                Console.WriteLine($"Working folder path: {workingFolder}");
            }
        }
        catch (DiffBuilderException e)
        {
            caughtException = true;

            WriteErrorMessage(e.Message);
        }

        if (caughtException)
        {
            Environment.Exit(1);
        }

        Console.WriteLine("Exit Code: {0}", Environment.ExitCode);

        if (Environment.ExitCode != 0)
        {
            Console.WriteLine("Overriding Exit Code to 0.");
        }

        Environment.Exit(0);
    }

    private static void WriteErrorMessage(string message)
    {
        var oldColor = Console.ForegroundColor;
        Console.ForegroundColor = ConsoleColor.Red;
        Console.WriteLine("Error: {0}", message);
        Console.ForegroundColor = oldColor;
    }

    private static string MakeLogFolder(string workingFolder)
    {
        if (!Directory.Exists(workingFolder))
        {
            Directory.CreateDirectory(workingFolder);
        }

        var baseLogFolder = Path.Combine(workingFolder, "logs");
        if (!Directory.Exists(baseLogFolder))
        {
            Directory.CreateDirectory(baseLogFolder);
        }

        const int MAX_LOG_FILES = 1000;

        for (int i = 0; i < MAX_LOG_FILES; i++)
        {
            var logFolder = Path.Combine(baseLogFolder, i.ToString());
            if (!Directory.Exists(logFolder))
            {
                Directory.CreateDirectory(logFolder);
                return logFolder;
            }
        }

        throw new Exception($"Could not create a log folder under {baseLogFolder} with {MAX_LOG_FILES} attempts.");
    }
}
