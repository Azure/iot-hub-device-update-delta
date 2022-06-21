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
    using System.IO;
    using System.Security.Cryptography;
    using System.Security.Authentication;

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

        public static Dictionary<HashAlgorithmType, string> GetHashesAndLengthOfStream(Stream stream, out UInt64 length)
        {
            Dictionary<HashAlgorithmType, string> hashes = new Dictionary<HashAlgorithmType, string>();
            UInt64 totalRead = 0;

            using (IncrementalHash sha256 = IncrementalHash.CreateHash(HashAlgorithmName.SHA256))
            {
                using (IncrementalHash md5 = IncrementalHash.CreateHash(HashAlgorithmName.MD5))
                {
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

                    hashes.Add(HashAlgorithmType.Sha256, ConvertBytesToHexadecimalString(sha256.GetCurrentHash()));
                    hashes.Add(HashAlgorithmType.Md5, ConvertBytesToHexadecimalString(md5.GetCurrentHash()));
                }
            }

            length = totalRead;
            return hashes;
        }

        public static Dictionary<HashAlgorithmType, string> CalculateHashes(BinaryReader reader, UInt64 length)
        {
            Dictionary<HashAlgorithmType, string> hashes = new();

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

            hashes.Add(HashAlgorithmType.Sha256, ConvertBytesToHexadecimalString(sha256.GetCurrentHash()));
            hashes.Add(HashAlgorithmType.Md5, ConvertBytesToHexadecimalString(md5.GetCurrentHash()));

            return hashes;
        }

        public static Dictionary<HashAlgorithmType, string> CalculateHashes(Stream stream)
        {
            return GetHashesAndLengthOfStream(stream, out UInt64 length);
        }

        public static Dictionary<HashAlgorithmType, string> CalculateHashes(ReadOnlySpan<byte> data)
        {
            Dictionary<HashAlgorithmType, string> hashes = new();

            using (var sha256 = SHA256.Create())
            {
                hashes.Add(HashAlgorithmType.Sha256, ConvertBytesToHexadecimalString(sha256.ComputeHash(data.ToArray())));
            }

            using (var md5 = MD5.Create())
            {
                hashes.Add(HashAlgorithmType.Md5, ConvertBytesToHexadecimalString(md5.ComputeHash(data.ToArray())));
            }

            return hashes;
        }

        public static bool CompareHashes(Dictionary<HashAlgorithmType, string> expected, Dictionary<HashAlgorithmType, string> actual)
        {
            return CompareHashes(expected, actual, out Tuple<HashAlgorithmType, string, string> failedHash);
        }

        public static bool CompareHashes(Dictionary<HashAlgorithmType, string> expected, Dictionary<HashAlgorithmType, string> actual, out Tuple<HashAlgorithmType, string, string> failedHash)
        {
            bool match = false;

            foreach (var hashType in actual.Keys)
            {
                if (!expected.ContainsKey(hashType))
                {
                    continue;
                }

                var expectedHash = expected[hashType];
                var actualHash = actual[hashType];

                if (!actualHash.Equals(expectedHash, StringComparison.OrdinalIgnoreCase))
                {
                    failedHash = new Tuple<HashAlgorithmType, string, string>(hashType, expectedHash, actualHash);
                    return false;
                }

                match = true;
            }

            failedHash = null;
            return match;
        }

        public static void ValidateFileHashes(string filePath, Dictionary<HashAlgorithmType, string> expectedHashes)
        {
            Dictionary<HashAlgorithmType, string> fileHashes;
            using (var readStream = File.OpenRead(filePath))
            {
                fileHashes = CalculateHashes(readStream);
            }

            if (!CompareHashes(expectedHashes, fileHashes, out Tuple<HashAlgorithmType, string, string> failedHash))
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
