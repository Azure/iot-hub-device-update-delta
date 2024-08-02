/**
 * @file Global.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace UnitTests;

using System.IO;
using System.IO.Compression;

using Microsoft.VisualStudio.TestTools.UnitTesting;

[TestClass]
public class Global
{
    [AssemblyInitialize]
    public static void Setup(TestContext testContext)
    {
        if (Directory.Exists("samples"))
        {
            Directory.Delete("samples", true);
        }

        ZipFile.ExtractToDirectory("samples.zip", ".");
    }
}
