namespace ArchiveUtility
{
    using System;
    using System.Diagnostics.CodeAnalysis;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public static class ChunkNames
    {
        public static string MakeHeaderChunkName(string payloadName)
        {
            return $"Chunk.Header.Payload={payloadName}";
        }

        public static string MakeChunkName(string payloadName)
        {
            return $"Chunk.Payload={payloadName}";
        }

        public static string MakePaddingChunkName(UInt64 offset)
        {
            return $"Chunk.Padding={offset}";
        }

        public const string GapChunkNamePrefix = "Chunk.Gap=";

        public static string MakeGapChunkName(UInt64 offset, UInt64 length)
        {
            return $"{GapChunkNamePrefix}{offset},{length}";
        }
    }
}
