/**
 * @file CpioArchiveBase.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using ArchiveUtility;

// Specializations of this type are documented here: https://www.systutorials.com/docs/linux/man/5-cpio/

namespace CpioArchives
{
    public abstract class CpioArchiveBase : VerbatimPayloadArchiveBase
    {
        public CpioArchiveBase(ArchiveLoaderContext context)
        {
            Context = context;
        }
        public override string ArchiveType { get { return "cpio"; } }
        protected bool TrailerDetected { get; set; } = false;
        protected abstract UInt64 AlignmentPadding { get; }
        protected override bool DoneTokenizing { get { return TrailerDetected; } }
        protected override bool SkipPayloadToken { get { return false; } }
        protected override Encoding StreamEncoding { get { return Encoding.ASCII; } }
        protected override UInt64 PayloadAlignment { get { return AlignmentPadding; } }
        protected const string TrailerFileName = "TRAILER!!!";
    }
}
