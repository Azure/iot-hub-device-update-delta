/**
 * @file SerializationTests.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace UnitTests;

using System;
using System.IO;
using System.Runtime.InteropServices;

using ArchiveUtility;

using Microsoft.VisualStudio.TestTools.UnitTesting;

[TestClass]
public class SerializationTests
{
    [DataRow("samples/archive_manifests/cpio1.swu.json")]
    [DataRow("samples/archive_manifests/cpio2.cpio.json")]
    [DataRow("samples/archive_manifests/cpio3.cpio.json")]
    [DataRow("samples/archive_manifests/cpio4.cpio.json")]
    [DataRow("samples/archive_manifests/cpio5.cpio.json")]
    [DataRow("samples/archive_manifests/cpio6.cpio.json")]
    [DataRow("samples/archive_manifests/ext4/sample1.ext4.json")]
    [DataRow("samples/archive_manifests/ext4/sample2.ext4.json")]
    [DataRow("samples/archive_manifests/ext4/sample3.ext4.json")]
    [DataTestMethod]
    public void Test_JsonManifestSerializationAndDeserialization(string jsonManifestPath)
    {
        string originalJson = File.ReadAllText(jsonManifestPath);
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            originalJson = originalJson.Replace("\r\n", "\n");
        }

        string reserializedJson = ArchiveTokenization.FromJson(originalJson).ToJson(true);

        var actualManifestPath = Path.GetFullPath(jsonManifestPath + ".actual");
        Console.WriteLine($"Saving actual results to: {actualManifestPath}");
        File.WriteAllText(actualManifestPath, reserializedJson);

        Assert.AreEqual(originalJson, reserializedJson);
    }
}
