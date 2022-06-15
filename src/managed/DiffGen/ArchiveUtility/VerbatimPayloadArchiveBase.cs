/**
 * @file VerbatimPayloadArchiveBase.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace ArchiveUtility
{
    public abstract class VerbatimPayloadArchiveBase : IArchive
    {
        protected ArchiveLoaderContext Context;
        protected abstract void ReadHeader(BinaryReader reader, UInt64 offset, out ArchiveItem chunk, out string payloadName, out UInt64 payloadLength);
        public bool IsMatchingFormat()
        {
            try
            {
                Context.Stream.Seek(0, SeekOrigin.Begin);
                using var reader = new BinaryReader(Context.Stream, Encoding.Default, true);
                ReadHeader(reader, 0, out ArchiveItem chunk, out string payloadName, out UInt64 payloadLength);

                if (payloadLength < 0)
                {
                    throw new FormatException("Payload length in first chunk is negative.");
                }

                UInt64 remainingStreamData = (UInt64) (reader.BaseStream.Length - reader.BaseStream.Position);
                if (payloadLength > remainingStreamData)
                {
                    throw new FormatException("Payload for first chunk extends past end of stream.");
                }

                return true;
            }
            catch (FormatException)
            {
            }
            catch (ArgumentException)
            {
            }
            return false;
        }
        protected abstract bool DoneTokenizing { get; }
        protected abstract bool SkipPayloadToken { get; }
        protected abstract Encoding StreamEncoding { get; }
        protected abstract UInt64 PayloadAlignment { get; }
        public ArchiveTokenization Tokenize()
        {
            if (Context.TokenCache.ContainsKey(GetType()))
            {
                return Context.TokenCache[GetType()];
            }

            ArchiveTokenization tokens = new(ArchiveType, ArchiveSubtype);

            Context.Stream.Seek(0, SeekOrigin.Begin);
            var length = (UInt64) Context.Stream.Length;

            using (var reader = new BinaryReader(Context.Stream, StreamEncoding, true))
            {
                UInt64 offset = 0;

                while (offset < length)
                {
                    ReadHeader(reader, offset, out ArchiveItem headerChunk, out string payloadName, out UInt64 payloadLength);
                    tokens.ArchiveItems.Add(headerChunk);

                    if (DoneTokenizing)
                    {
                        break;
                    }

                    offset += headerChunk.Length;

                    if (SkipPayloadToken)
                    {
                        continue;
                    }

                    ReadPayload(reader, payloadLength, payloadName, out ArchiveItem payload);

                    var payloadChunkName = ArchiveItem.MakeChunkName(payloadName);
                    ArchiveItem payloadChunk = new(payloadChunkName, ArchiveItemType.Chunk, (UInt64)offset, (UInt64)payloadLength, null, payload.Hashes);

                    Recipe payloadRecipe = new CopyRecipe(payloadChunk.MakeReference());
                    Recipe chunkRecipe = new CopyRecipe(payload.MakeReference());

                    payloadChunk.Recipes.Add(chunkRecipe);
                    payload.Recipes.Add(payloadRecipe);

                    tokens.ArchiveItems.Add(payloadChunk);
                    tokens.ArchiveItems.Add(payload);

                    offset += payloadLength;

                    UInt64 paddingNeeded = BinaryData.GetPaddingNeeded(offset, PayloadAlignment);
                    if (paddingNeeded != 0)
                    {
                        var name = ArchiveItem.MakePaddingChunkName(offset);
                        var paddingChunk = ArchiveItem.FromBinaryReader(name, ArchiveItemType.Chunk, reader, paddingNeeded, offset);
                        tokens.ArchiveItems.Add(paddingChunk);
                        offset += paddingNeeded;
                    }
                }
            }

            Context.TokenCache[GetType()] = tokens;
            return tokens;
        }
        public abstract string ArchiveType { get; }
        public abstract string ArchiveSubtype { get; }
        protected static void ReadPayload(BinaryReader reader, UInt64 payloadLength, string payloadName, out ArchiveItem payload)
        {
            payload = ArchiveItem.FromBinaryReader(payloadName, ArchiveItemType.Payload, reader, payloadLength);
        }
    }
}
