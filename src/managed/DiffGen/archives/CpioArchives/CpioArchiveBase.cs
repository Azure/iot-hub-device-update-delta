/**
 * @file CpioArchiveBase.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace CpioArchives
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Text;

    using ArchiveUtility;

    // Specializations of this type are documented here: https://www.systutorials.com/docs/linux/man/5-cpio/
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public abstract class CpioArchiveBase : VerbatimPayloadArchiveBase
    {
        public CpioArchiveBase(ArchiveLoaderContext context)
            : base(context)
        {
        }

        public override string ArchiveType
        {
            get { return "cpio"; }
        }

        protected bool TrailerDetected { get; set; } = false;

        protected abstract UInt64 AlignmentPadding { get; }

        protected override bool DoneTokenizing { get => TrailerDetected; }

        protected override bool SkipPayloadToken { get => false; }

        protected override Encoding StreamEncoding { get => Encoding.ASCII; }

        protected override UInt64 PayloadAlignment { get => AlignmentPadding; }

        protected const string TrailerFileName = "TRAILER!!!";
    }
}
