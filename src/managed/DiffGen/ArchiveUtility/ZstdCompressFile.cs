/**
 * @file ZstdCompressFile.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Threading;

    using Microsoft.Extensions.Logging;

    internal class ZstdCompressFile
    {
        public static string ZSTD_COMPRESS_FILE_EXE_PATH
        {
            get
            {
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    return "zstd_compress_file";
                }

                return "zstd_compress_file.exe";
            }
        }

        public static string BinaryPath { get; } = ZSTD_COMPRESS_FILE_EXE_PATH;

        public static string FormatArgs(string source, string target, string delta)
        {
            if (string.IsNullOrEmpty(source))
            {
                return $@"""{target}"" ""{delta}""";
            }
            else
            {
                return $@"""{target}"" ""{source}"" ""{delta}""";
            }
        }

        static void Call(string source, string target, string delta)
        {
            using (Process process = new())
            {
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.FileName = ProcessHelper.GetPathInRunningDirectory(BinaryPath);
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.Arguments = FormatArgs(source, target, delta);

                process.StartAndReport();
                process.WaitForExit();

                if (!process.HasExited)
                {
                    process.Kill(true);
                }
            }
        }

        public static void CompressFile(string uncompressed, string compressed)
        {
            Call(null, uncompressed, compressed);
        }

        public static void CreateDelta(string source, string target, string delta)
        {
            Call(source, target, delta);
        }
    }
}
