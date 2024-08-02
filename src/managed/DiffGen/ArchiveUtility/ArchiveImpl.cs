namespace ArchiveUtility
{
    using System;
    using System.IO;
    using System.Text;

    public abstract class ArchiveImpl : IArchive
    {
        public abstract string ArchiveType { get; }

        public abstract string ArchiveSubtype { get; }

        protected ArchiveLoaderContext Context { get; init; }

        private bool hasTriedProcessing = false;
        private ArchiveTokenization tokens = null;

        public ArchiveImpl(ArchiveLoaderContext context)
        {
            Context = context;
        }

        public virtual bool IsMatchingFormat()
        {
            if (Context.TokenCache.ContainsKey(GetType()))
            {
                return true;
            }

            PopulateTokens();

            if (tokens is null)
            {
                return false;
            }

            Context.TokenCache[GetType()] = tokens;

            return true;
        }

        public ArchiveTokenization Tokenize()
        {
            PopulateTokens();

            if (tokens == null)
            {
                throw new Exception($"Couldn't parse stream as {ArchiveType}.{ArchiveSubtype}");
            }

            return tokens;
        }

        private void PopulateTokens()
        {
            if (Context.ArchiveItem is null)
            {
                Context.Stream.Seek(0, SeekOrigin.Begin);
                using var reader = new BinaryReader(Context.Stream, Encoding.Default, true);
                Context.ArchiveItem = ItemDefinition.FromBinaryReader(reader, (ulong)Context.Stream.Length);
            }

            if (!hasTriedProcessing)
            {
                if (TryTokenize(Context.ArchiveItem, out tokens))
                {
                    if (tokens == null)
                    {
                        throw new Exception("TryTokenize() succeeded, but tokenization result was null.");
                    }

                    PostProcess(tokens, Context);

                    Context.TokenCache[GetType()] = tokens;
                }

                hasTriedProcessing = true;
            }
        }

        public abstract bool TryTokenize(ItemDefinition archiveItem, out ArchiveTokenization tokens);

        public virtual void PostProcess(ArchiveTokenization tokens, ArchiveLoaderContext context)
        {
            tokens.WorkingFolder = context.WorkingFolder;
            tokens.HandleGapChunks(context.Stream);
            tokens.ProcessNestedArchives(context.Stream, context.UseCase);
        }
    }
}
