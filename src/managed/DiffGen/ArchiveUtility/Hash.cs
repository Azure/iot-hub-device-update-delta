/**
 * @file Hash.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.IO;
    using System.Security.Authentication;
    using System.Security.Cryptography;

    public record Hash(HashAlgorithmType Type, byte[] Value)
    {
        public string ValueString()
        {
            return HexUtility.ByteArrayToHexString(Value);
        }

        public override string ToString()
        {
            return ValueString() + "(" + Type.ToString() + ")";
        }

        public static string Sha256FileAsString(string path)
        {
            return HexUtility.ByteArrayToHexString(Sha256File(path));
        }
        public static byte[] Sha256File(string path)
        {
            const int READ_BUFFER_SIZE = 4096;
            byte[] readBuffer = new byte[READ_BUFFER_SIZE];
            using (var hasher = IncrementalHash.CreateHash(HashAlgorithmName.SHA256))
            {
                using (var stream = File.OpenRead(path))
                {
                    while (stream.Length != stream.Position)
                    {
                        var actualRead = stream.Read(readBuffer, 0, readBuffer.Length);
                        var span = new ReadOnlySpan<byte>(readBuffer, 0, actualRead);
                        hasher.AppendData(span);
                    }
                }
                return hasher.GetCurrentHash();
            }
        }
        public static Hash FromFile(string path)
        {
            return new Hash(HashAlgorithmType.Sha256, Sha256File(path));
        }
    }
}
