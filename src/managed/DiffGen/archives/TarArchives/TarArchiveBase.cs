/**
 * @file TarArchiveBase.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace TarArchives
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Text;

    using ArchiveUtility;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public abstract class TarArchiveBase : VerbatimPayloadArchiveBase
    {
        protected const int TarBlockSize = 512;

        protected TarArchiveBase(ArchiveLoaderContext context)
            : base(context)
        {
        }

        public override string ArchiveType { get => "tar"; }

        protected int EmptyBlocks { get; set; } = 0;

        protected override bool DoneTokenizing { get => EmptyBlocks == 2; }

        protected override bool SkipPayloadToken { get => EmptyBlocks != 0; }

        protected override Encoding StreamEncoding { get => Encoding.ASCII; }

        protected override UInt64 PayloadAlignment { get => TarBlockSize; }
    }
}
