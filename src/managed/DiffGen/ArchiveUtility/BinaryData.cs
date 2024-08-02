/**
 * @file BinaryData.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Linq;
    using System.Security.Authentication;
    using System.Security.Cryptography;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class BinaryData
    {
        public static UInt64 GetPaddingNeeded(UInt64 offset, UInt64 alignment)
        {
            if (alignment == 0)
            {
                return 0;
            }

            UInt64 paddingNeeded = alignment - (offset % alignment);
            if (paddingNeeded == alignment)
            {
                paddingNeeded = 0;
            }

            return paddingNeeded;
        }

        public static string ConvertBytesToHexadecimalString(byte[] data)
        {
            return BitConverter.ToString(data).Replace("-", string.Empty);
        }

        public static Dictionary<HashAlgorithmType, Hash> GetHashesAndLengthOfStream(Stream stream, out UInt64 length)
        {
            Dictionary<HashAlgorithmType, Hash> hashes = new();
            UInt64 totalRead = 0;

            using IncrementalHash sha256 = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
            using IncrementalHash md5 = IncrementalHash.CreateHash(HashAlgorithmName.MD5);

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
                sha256.AppendData(data, 0, bytesRead);
                md5.AppendData(data, 0, bytesRead);
            }

            hashes.Add(HashAlgorithmType.Sha256, new Hash(HashAlgorithmType.Sha256, sha256.GetCurrentHash()));
            hashes.Add(HashAlgorithmType.Md5, new Hash(HashAlgorithmType.Md5, md5.GetCurrentHash()));

            length = totalRead;
            return hashes;
        }

        public static Dictionary<HashAlgorithmType, Hash> CalculateHashes(BinaryReader reader, UInt64 length)
        {
            Dictionary<HashAlgorithmType, Hash> hashes = new();

            using IncrementalHash sha256 = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
            using IncrementalHash md5 = IncrementalHash.CreateHash(HashAlgorithmName.MD5);

            ulong blockSize = 1024 * 64;
            byte[] data = new byte[blockSize];
            var remaining = length;
            while (remaining > 0)
            {
                var toRead = (int)Math.Min(blockSize, remaining);
                int bytesRead = reader.Read(data, 0, toRead);
                if (bytesRead == 0)
                {
                    break;
                }

                remaining -= (UInt64)bytesRead;
                sha256.AppendData(data, 0, bytesRead);
                md5.AppendData(data, 0, bytesRead);
            }

            if (remaining != 0)
            {
                throw new Exception("Coulnd't read expected data for hashing.");
            }

            hashes.Add(HashAlgorithmType.Sha256, new Hash(HashAlgorithmType.Sha256, sha256.GetCurrentHash()));
            hashes.Add(HashAlgorithmType.Md5, new Hash(HashAlgorithmType.Md5, md5.GetCurrentHash()));

            return hashes;
        }

        public static Dictionary<HashAlgorithmType, Hash> CalculateHashes(Stream stream)
        {
            return GetHashesAndLengthOfStream(stream, out UInt64 length);
        }

        public static Dictionary<HashAlgorithmType, Hash> CalculateHashes(ReadOnlySpan<byte> data)
        {
            Dictionary<HashAlgorithmType, Hash> hashes = new();

            using (var sha256 = SHA256.Create())
            {
                Hash hash = new(HashAlgorithmType.Sha256, sha256.ComputeHash(data.ToArray()));
                hashes.Add(HashAlgorithmType.Sha256, hash);
            }

            using (var md5 = MD5.Create())
            {
                Hash hash = new(HashAlgorithmType.Md5, md5.ComputeHash(data.ToArray()));
                hashes.Add(HashAlgorithmType.Md5, hash);
            }

            return hashes;
        }

        public static bool CompareHashes(Dictionary<HashAlgorithmType, Hash> expected, Dictionary<HashAlgorithmType, Hash> actual)
        {
            return CompareHashes(expected, actual, out Tuple<HashAlgorithmType, byte[], byte[]> failedHash);
        }

        public static bool CompareHashes(Dictionary<HashAlgorithmType, Hash> expected, Dictionary<HashAlgorithmType, Hash> actual, out Tuple<HashAlgorithmType, byte[], byte[]> failedHash)
        {
            bool match = false;

            foreach (var h in actual)
            {
                if (expected.ContainsKey(h.Key))
                {
                    if (!h.Value.Equals(expected[h.Key]))
                    {
                        failedHash = new Tuple<HashAlgorithmType, byte[], byte[]>(h.Key, expected[h.Key].Value.ToArray(), h.Value.Value.ToArray());
                        return false;
                    }
                    else
                    {
                        match = true;
                    }
                }
            }

            failedHash = null;
            return match;
        }

        public static void ValidateFileHashes(string filePath, Dictionary<HashAlgorithmType, Hash> expectedHashes)
        {
            Dictionary<HashAlgorithmType, Hash> fileHashes;
            using (var readStream = File.OpenRead(filePath))
            {
                fileHashes = CalculateHashes(readStream);
            }

            if (!CompareHashes(expectedHashes, fileHashes, out Tuple<HashAlgorithmType, byte[], byte[]> failedHash))
            {
                if (failedHash == null)
                {
                    throw new Exception($"Couldn't find a matching hash for {filePath}");
                }

                throw new Exception($"Hash mismatch for {filePath}. Hash type: {failedHash.Item1}, Expected: {failedHash.Item2}, Actual: {failedHash.Item3}");
            }
        }
    }
}
