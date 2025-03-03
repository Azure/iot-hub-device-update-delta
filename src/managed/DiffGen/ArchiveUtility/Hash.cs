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
    using System.Linq;
    using System.Security.Authentication;
    using System.Security.Cryptography;

    public class Hash
    {
        public HashAlgorithmType Type { get; init; }

        public byte[] Value { get; init; }

        public Hash(HashAlgorithmType type, byte[] value)
        {
            Type = type;
            Value = value;
        }

        public string ValueString()
        {
            return HexUtility.ByteArrayToHexString(Value.ToArray());
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

        public override int GetHashCode()
        {
            int result = Type.GetHashCode();

            foreach (var b in Value)
            {
                result = HashCode.Combine(result, b.GetHashCode());
            }

            return result;
        }

        public static bool Equals(Hash x, Hash y)
        {
            if (x.Type != y.Type)
            {
                return false;
            }

            return Enumerable.SequenceEqual(x.Value, y.Value);
        }

        public override bool Equals(object obj)
        {
            if (obj is null)
            {
                return false;
            }

            if (obj is Hash)
            {
                return Equals(this, (Hash)obj);
            }

            return base.Equals(obj);
        }
    }
}
