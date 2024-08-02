/**
 * @file ItemDefinition.cs
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
    using System.Threading;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class ItemDefinition : IEquatable<ItemDefinition>, IComparable<ItemDefinition>
    {
        public Dictionary<HashAlgorithmType, Hash> Hashes { get; set; }

        public List<string> Names { get; set; }

        public UInt64 Length { get; set; }

        public ItemDefinition()
        {
        }

        public ItemDefinition(UInt64 length)
        {
            Length = length;
        }

        public ItemDefinition(UInt64 length, Dictionary<HashAlgorithmType, Hash> hashes, List<string> names)
        {
            Length = length;
            Hashes = hashes;
            Names = names;
        }

        public ItemDefinition WithHash(Hash hash)
        {
            Dictionary<HashAlgorithmType, Hash> hashes = new();
            List<string> names = new();
            foreach (var typeAndHash in Hashes)
            {
                hashes.Add(typeAndHash.Key, typeAndHash.Value);
            }

            hashes.Add(hash.Type, hash);

            var newItem = new ItemDefinition(Length, hashes, names);
            return newItem;
        }

        public Hash GetSha256Hash()
        {
            if (!Hashes.ContainsKey(HashAlgorithmType.Sha256))
            {
                return null;
            }

            return Hashes[HashAlgorithmType.Sha256];
        }

        public string GetSha256HashString()
        {
            if (!Hashes.ContainsKey(HashAlgorithmType.Sha256))
            {
                return null;
            }

            return Hashes[HashAlgorithmType.Sha256].ValueString();
        }

        public ItemDefinition WithName(string name)
        {
            Dictionary<HashAlgorithmType, Hash> hashes = new();
            List<string> names = new();
            foreach (var typeAndHash in Hashes)
            {
                hashes.Add(typeAndHash.Key, typeAndHash.Value);
            }

            names.AddRange(Names);
            names.Add(name);

            var newItem = new ItemDefinition(Length, hashes, names);
            return newItem;
        }

        public ItemDefinition WithoutNames()
        {
            if (Names.Count == 0)
            {
                return this;
            }

            return new(Length, Hashes, new());
        }

        public bool IsGapChunk()
        {
            return Names.Any(x => x.StartsWith(ChunkNames.GapChunkNamePrefix));
        }

        public override string ToString()
        {
            string value = "{len: " + Length.ToString() + ", Hashes: {";

            foreach (var hash in Hashes)
            {
                value += hash.Value.ValueString() + ", ";
            }

            value += "}";

            if (Names.Count > 0)
            {
                value += ", Names: { ";

                foreach (var name in Names)
                {
                    value += "\"" + name + "\",";
                }

                value += "}";
            }

            value += "}";

            return value;
        }

        public int CompareTo(ItemDefinition other)
        {
            return Compare(this, other);
        }

        public static int Compare(ItemDefinition x, ItemDefinition y)
        {
            if (x.Length != y.Length)
            {
                return (x.Length < y.Length) ? -1 : 1;
            }

            if (x.Hashes.Count != y.Hashes.Count)
            {
                return x.Hashes.Count - y.Hashes.Count;
            }

            if ((x.Hashes.Count == 0) && (x.Names.Count != y.Names.Count))
            {
                return x.Names.Count - y.Names.Count;
            }

            var sortedXHashes = x.Hashes.OrderBy(x => x.Key);
            var sortedYHashes = y.Hashes.OrderBy(y => y.Key);

            foreach (var hashesPair in sortedXHashes.Zip(sortedYHashes))
            {
                if (hashesPair.First.Key != hashesPair.Second.Key)
                {
                    return hashesPair.First.Key.CompareTo(hashesPair.Second.Key);
                }

                byte[] hashXValue = hashesPair.First.Value.Value;
                byte[] hashYValue = hashesPair.Second.Value.Value;

                foreach (var hashBytePair in hashXValue.Zip(hashYValue))
                {
                    if (hashBytePair.First != hashBytePair.Second)
                    {
                        return hashBytePair.First - hashBytePair.Second;
                    }
                }
            }

            if (x.Hashes.Count == 0)
            {
                var sortedXNames = x.Names.OrderBy(x => x);
                var sortedYNames = y.Names.OrderBy(x => x);

                foreach (var namePair in sortedXNames.Zip(sortedYNames))
                {
                    var nameCompareResult = namePair.First.CompareTo(namePair.Second);

                    if (nameCompareResult != 0)
                    {
                        return nameCompareResult;
                    }
                }
            }

            return 0;
        }

        public override int GetHashCode()
        {
            int result = Length.GetHashCode();
            foreach (var entry in Hashes.OrderBy(x => x.Key))
            {
                result = HashCode.Combine(result, entry.Key.GetHashCode(), entry.Value.GetHashCode());
            }

            foreach (var name in Names)
            {
                result = HashCode.Combine(result, name.GetHashCode());
            }

            return result;
        }

        public static bool Equals(ItemDefinition x, ItemDefinition y)
        {
            if (x.Length != y.Length)
            {
                return false;
            }

            if (x.Hashes.Count != y.Hashes.Count)
            {
                return false;
            }

            if ((x.Hashes.Count == 0) && (x.Names.Count != y.Names.Count))
            {
                return false;
            }

            foreach (var entry in x.Hashes)
            {
                var hashType = entry.Key;

                if (!y.Hashes.ContainsKey(hashType))
                {
                    return false;
                }

                var xHash = entry.Value;
                var yHash = y.Hashes[hashType];
                if (!Enumerable.SequenceEqual(xHash.Value, yHash.Value))
                {
                    return false;
                }
            }

            if (x.Hashes.Count == 0)
            {
                return Enumerable.SequenceEqual(x.Names, y.Names);
            }

            return true;
        }

        public bool Equals(ItemDefinition other)
        {
            return Equals(this, other);
        }

        public override bool Equals(object obj)
        {
            if (obj is ItemDefinition)
            {
                return Equals(this, (ItemDefinition)obj);
            }

            return base.Equals(obj);
        }

        public static ItemDefinition FromBinaryReader(BinaryReader reader, UInt64 length)
        {
            var hashes = BinaryData.CalculateHashes(reader, length);
            List<string> names = new();

            var newItem = new ItemDefinition(length, hashes, names);

            return newItem;
        }

        public static ItemDefinition FromByteSpan(ReadOnlySpan<byte> data)
        {
            var hashes = BinaryData.CalculateHashes(data);

            return new ItemDefinition((UInt64)data.Length, hashes, new List<string>());
        }

        private static int GetMaxRetryCount(string path)
        {
            const long maxRetryBase = 10;
            long fileSize = new FileInfo(path).Length;
            long tenMegabytes = 10 * 1024 * 1024;

            return (int)Math.Min(20L, maxRetryBase + (fileSize / tenMegabytes));
        }

        public static ItemDefinition FromFile(string path)
        {
            FileInfo deltaFileInfo = new(path);
            if (deltaFileInfo.Length < 0)
            {
                throw new Exception($"File length was negative for file: {path}");
            }

            int retries = 0;

            Exception lastException = null;

            int maxRetries = GetMaxRetryCount(path);

            int sleepTimeMilliseconds = 1000;

            while (retries++ < maxRetries)
            {
                try
                {
                    using var stream = File.Open(path, FileMode.Open, FileAccess.Read);
                    using var reader = new BinaryReader(stream);

                    return FromBinaryReader(reader, (ulong)deltaFileInfo.Length);
                }
                catch (IOException e)
                {
                    lastException = e;
                    Thread.Sleep(sleepTimeMilliseconds);
                    sleepTimeMilliseconds += 1000;
                }
            }

            throw new Exception($"Failed to get item from file: {path}. {lastException.Message}", lastException);
        }

        public string GetExtractionPath(string root)
        {
            return Path.Combine(root, GetSha256HashString());
        }
    }
}
