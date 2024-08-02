/**
 * @file VerbatimPayloadArchiveBase.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility;

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Text;

[SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
public abstract class VerbatimPayloadArchiveBase : ArchiveImpl
{
    public VerbatimPayloadArchiveBase(ArchiveLoaderContext context)
        : base(context)
    {
    }

    public record HeaderDetails(ItemDefinition Item, Recipe Recipe, string PayloadName, UInt64 PayloadLength)
    {
        public HeaderDetails()
            : this(null, null, string.Empty, 0)
        {
        }
    }

    protected abstract HeaderDetails ReadHeader(BinaryReader reader);

    public override bool IsMatchingFormat()
    {
        try
        {
            Context.Stream.Seek(0, SeekOrigin.Begin);
            using var reader = new BinaryReader(Context.Stream, Encoding.Default, true);
            var headerDetails = ReadHeader(reader);

            if (headerDetails.PayloadLength < 0)
            {
                // Payload length in first chunk is negative.
                return false;
            }

            UInt64 remainingStreamData = (UInt64)(reader.BaseStream.Length - reader.BaseStream.Position);
            if (headerDetails.PayloadLength > remainingStreamData)
            {
                // Payload for first chunk extends past end of stream.
                return false;
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

    protected void AddChunk(ArchiveTokenization tokens, ulong offset, ItemDefinition item)
    {
        Recipe chunkRecipe = new(RecipeType.Slice, item, new() { offset }, new List<ItemDefinition>() { tokens.ArchiveItem });
        tokens.AddReverseRecipe(chunkRecipe);
    }

    private void ReadChunk(BinaryReader reader, ArchiveTokenization tokens, ref UInt64 offset, out bool doneTokenizing)
    {
        var headerDetails = ReadHeader(reader);

        var headerItem = headerDetails.Item;

        AddChunk(tokens, offset, headerItem);

        offset += headerItem.Length;

        if (headerDetails.Recipe != null)
        {
            tokens.AddRecipe(headerDetails.Recipe);
        }

        if (DoneTokenizing)
        {
            doneTokenizing = true;
            return;
        }

        if (SkipPayloadToken)
        {
            doneTokenizing = false;
            return;
        }

        var payloadItem = ReadPayload(reader, headerDetails.PayloadLength, headerDetails.PayloadName);
        tokens.AddRootPayload(headerDetails.PayloadName, payloadItem);

        AddChunk(tokens, offset, payloadItem);

        offset += payloadItem.Length;

        UInt64 paddingNeeded = BinaryData.GetPaddingNeeded(offset, PayloadAlignment);
        if (paddingNeeded != 0)
        {
            var name = ChunkNames.MakePaddingChunkName(offset);
            var paddingItem = ItemDefinition.FromBinaryReader(reader, paddingNeeded).WithName(name);

            AddChunk(tokens, offset, paddingItem);
            offset += paddingItem.Length;
        }

        doneTokenizing = false;
    }

    public override bool TryTokenize(ItemDefinition archiveItem, out ArchiveTokenization tokens)
    {
        ArchiveTokenization newTokens = new(ArchiveType, ArchiveSubtype);

        newTokens.ArchiveItem = archiveItem;
        Context.Stream.Seek(0, SeekOrigin.Begin);
        var length = (UInt64)Context.Stream.Length;

        using var reader = new BinaryReader(Context.Stream, StreamEncoding, true);

        UInt64 offset = 0;

        while (offset < length)
        {
            ReadChunk(reader, newTokens, ref offset, out bool doneTokenizing);

            if (doneTokenizing)
            {
                break;
            }
        }

        tokens = newTokens;
        return true;
    }

    protected static ItemDefinition ReadPayload(BinaryReader reader, UInt64 payloadLength, string payloadName)
    {
        return ItemDefinition.FromBinaryReader(reader, payloadLength).WithName(payloadName);
    }
}
