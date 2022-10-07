/**
 * @file DiffSerializer.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System.IO;
    using ArchiveUtility;

    class DiffSerializer
    {
        public static void WriteDiff(Diff diff, string path)
        {
            ADUCreate.Session session = new();
            foreach (var chunk in diff.Chunks)
            {
                session.AddChunk(chunk);
            }

            var inlineAssetsPath = path + ".inline_assets";
            using (var inlineAssetsStream = File.Create(inlineAssetsPath))
            {
                foreach (var onDeviceArchiveItem in diff.GetAllInlineAssetArchiveItems())
                {
                    var devicePath = onDeviceArchiveItem.DevicePath;

                    using (var inlineAssetStream = File.OpenRead(devicePath))
                    {
                        inlineAssetStream.CopyTo(inlineAssetsStream);
                    }
                }
            }

            session.SetTargetSizeAndHash(diff.TargetSize, diff.TargetHash);
            session.SetSourceSizeAndHash(diff.SourceSize, diff.SourceHash);
            session.SetInlineAssetPath(inlineAssetsPath);
            session.SetRemainderPath(diff.RemainderPath);
            session.SetRemainderSizes(diff.RemainderUncompressedSize, diff.RemainderCompressedSize);

            session.Write(path);
        }
    }
}
