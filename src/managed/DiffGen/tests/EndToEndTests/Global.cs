/**
 * @file Global.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace EndToEndTests;

using System.Diagnostics.Contracts;
using System.IO;
using System.IO.Compression;

using Microsoft.VisualStudio.TestTools.UnitTesting;

[TestClass]
public class Global
{
    [AssemblyInitialize]
    public static void Setup(TestContext context)
    {
        if (Directory.Exists("samples"))
        {
            Directory.Delete("samples", true);
        }

        ZipFile.ExtractToDirectory("samples.zip", ".");
    }
}
