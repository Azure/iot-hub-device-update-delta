/**
 * @file Program.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
// See https://aka.ms/new-console-template for more information

using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;

void Usage()
{
    var mainModule = Process.GetCurrentProcess().MainModule;
    var exeName = (mainModule == null) ? "FixupSources" : mainModule.FileName;
    Console.WriteLine("Usage: {0} <path>", exeName);
}

bool IsCSharpCodeFile(string path)
{
    return path.EndsWith(".cs");
}

bool IsCppCodeFile(string path)
{
    return path.EndsWith(".cpp") || path.EndsWith(".c") || path.EndsWith(".h");
}

bool IsGeneratedFile(string path)
{
    return path.Contains("obj") || path.Contains("\\out");
}

bool IsCodeFile(string path)
{
    return IsCSharpCodeFile(path) || IsCppCodeFile(path);
}

void ProcessCppCodeFile(string path)
{
    Console.WriteLine("Processing Cpp Code File: {0}", path);

    bool changed = false;

    string copyrightComment = CreateMsftCopyrightComment(path);

    string allFileText = File.ReadAllText(path);

    if (!allFileText.StartsWith(copyrightComment))
    {
        StripBadLicenseComment(ref allFileText);
        allFileText = copyrightComment + allFileText;
        changed = true;
    }
    else
    {
        Console.WriteLine("File has correct copyright comment.");
    }

    if (FixRootCppNamespace(ref allFileText))
    {
        changed = true;
    }

    if (changed)
    {
        Console.WriteLine("Updating file.");
        File.WriteAllText(path, allFileText);
    }
}

string CreateMsftCopyrightComment(string path)
{
    var fileName = Path.GetFileName(path);
    return $"/**\r\n * @file {fileName}\r\n *\r\n * @copyright Copyright (c) Microsoft Corporation.\r\n * Licensed under the MIT License.\r\n */\r\n";
}

bool StripBadLicenseComment(ref string text)
{
    if (!text.StartsWith("/*"))
    {
        return false;
    }

    int commentLength = 0;

    for (int i = 2; i + 1 < text.Length; i++)
    {
        if (text[i] == '*' && text[i + 1] == '/')
        {
            commentLength = i + 2;
            break;
        }
    }

    if (commentLength == 0)
    {
        return false;
    }

    string commentText = text.Substring(0, commentLength);
    if (!commentText.Contains("copyright", StringComparison.OrdinalIgnoreCase))
    {
        return false;
    }

    text = text.Substring(commentLength);
    return true;
}

string[] CHILDREN_OF_ROOT = { "diffs", "errors", "hashing", "io", "test_utility" };

bool FixRootCppNamespace(ref string text)
{
    int startingHashCode = text.GetHashCode();

    string ROOT_NAMESPACE = "archive_diff";
    string validChildren = string.Join("|", CHILDREN_OF_ROOT);

    text = Regex.Replace(text, @$"(namespace\s+)(.*::)?({validChildren})", $"$1{ROOT_NAMESPACE}::$3");

    if (!text.Contains($"namespace {ROOT_NAMESPACE}"))
    {
        text = Regex.Replace(text, @$"([a-zA-Z0-9_]*::)?({validChildren})::", $"{ROOT_NAMESPACE}::$2::");
    }

    return text.GetHashCode() != startingHashCode;
}

void ProcessCSharpCodeFile(string path)
{
    bool changed = false;
    Console.WriteLine("Processing C# Code File: {0}", path);

    string copyrightComment = CreateMsftCopyrightComment(path);

    string allFileText = File.ReadAllText(path);

    if (!allFileText.StartsWith(copyrightComment))
    {
        StripBadLicenseComment(ref allFileText);
        allFileText = copyrightComment + allFileText;
        changed = true;
    }
    else
    {
        Console.WriteLine("File has correct copyright comment.");
    }

    if (changed)
    {
        Console.WriteLine("Updating file.");
        File.WriteAllText(path, allFileText);
    }
}

if (args.Length < 1)
{
    Usage();
    Environment.Exit(1);
}

var path = args[0];

Console.WriteLine("Processing source files under path: {0}", path);

if (!Directory.Exists(path))
{
    Console.WriteLine("{0} doesn't exist.", path);
    Environment.Exit(1);
}

var files = Directory.EnumerateFiles(path, "*.*", SearchOption.AllDirectories).Where(file => IsCodeFile(file) && !IsGeneratedFile(file));

foreach (var file in files)
{
    if (IsCppCodeFile(file))
    {
        ProcessCppCodeFile(file);
    }
    else if (IsCSharpCodeFile(file))
    {
        ProcessCSharpCodeFile(file);
    }
}