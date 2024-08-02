/**
 * @file TestUtility.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace UnitTests;

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

[SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
public class TestUtility
{
    public static void DumpFolderHash(string path)
    {
        var hash = TestUtility.HashFolderContents(path, out UInt64 folderTotalBytes);
        var hashString = StringizeByteArray(hash);
        Console.WriteLine($"{path} hash: {hashString}, total bytes: {folderTotalBytes}");
    }

    public static string StringizeByteArray(byte[] data)
    {
        return BitConverter.ToString(data).Replace("-", string.Empty);
    }

    public static byte[] HashFolderContents(string path, out UInt64 totalBytes)
    {
        using IncrementalHash hasher = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);

        IncrementalHashFolderContent(hasher, path, out totalBytes);

        return hasher.GetCurrentHash();
    }

    public static void IncrementalHashFolderContent(IncrementalHash hasher, string path, out UInt64 totalBytes)
    {
        var directories = Directory.EnumerateDirectories(path);
        var sortedDirectories = directories.ToArray().OrderBy(d => d);
        totalBytes = 0;

        foreach (var d in sortedDirectories)
        {
            IncrementalHashFolderContent(hasher, d, out UInt64 folderBytes);
            totalBytes += folderBytes;
        }

        var files = Directory.EnumerateFiles(path);
        var sortedFiles = files.ToArray().OrderBy(f => f);
        foreach (var f in sortedFiles)
        {
            IncrementalHashFile(hasher, f, out UInt64 fileBytes);
            totalBytes += fileBytes;
        }
    }

    public static void IncrementalHashFile(IncrementalHash hasher, string path, out UInt64 length)
    {
        using var stream = File.OpenRead(path);
        UInt64 totalRead = 0;

        int blockSize = 1024 * 64;
        byte[] data = new byte[blockSize];
        while (true)
        {
            int bytesRead = stream.Read(data, 0, blockSize);
            if (bytesRead == 0)
            {
                break;
            }

            totalRead += (UInt64)bytesRead;
            hasher.AppendData(data, 0, bytesRead);
        }

        length = totalRead;
    }

    public record LineDifference(string Expected, string Actual, int LineNumber)
    {
        public override string ToString()
        {
            return $"{LineNumber}) Expected: \"{Expected}\", Actual: \"{Actual}\"";
        }
    }

    public static List<LineDifference> GetLineDifferences(string expected, string actual)
    {
        using StringReader expectedReader = new StringReader(expected);
        using StringReader actualReader = new StringReader(actual);
        int lineNumber = 0;

        List<LineDifference> differences = new();

        while (true)
        {
            string expectedLine = expectedReader.ReadLine();
            string actualLine = actualReader.ReadLine();

            lineNumber++;

            if (actualLine == null && expectedLine == null)
            {
                break;
            }

            if (expectedLine == null)
            {
                differences.Add(new(string.Empty, actualLine, lineNumber));
                continue;
            }
            else if (actualLine == null)
            {
                differences.Add(new(expectedLine, string.Empty, lineNumber));
                continue;
            }

            if (expectedLine.Equals(actualLine))
            {
                continue;
            }

            differences.Add(new (expectedLine, actualLine, lineNumber));
        }

        return differences;
    }

    public static string GetStringDifferences(string expected, string actual)
    {
        var differences = GetLineDifferences(expected, actual);

        StringBuilder differentLinesStringBuilder = new();
        foreach (var diff in differences)
        {
            differentLinesStringBuilder.AppendLine(diff.ToString());
        }

        return differentLinesStringBuilder.ToString();
    }

    public static bool CompareMultilineStrings(string expected, string actual)
    {
        var differences = GetStringDifferences(expected, actual);

        return string.Empty.Equals(differences);
    }
}