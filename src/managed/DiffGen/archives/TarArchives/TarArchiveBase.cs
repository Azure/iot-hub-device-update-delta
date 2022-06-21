/**
 * @file TarArchiveBase.cs
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

namespace TarArchives
{
    public abstract class TarArchiveBase : VerbatimPayloadArchiveBase
    {
        protected TarArchiveBase(ArchiveLoaderContext context)
        {
            Context = context;
        }

        public override string ArchiveType { get { return "tar"; } }

        protected int EmptyBlocks { get; set; } = 0;
        protected const int TarBlockSize = 512;

        protected override bool DoneTokenizing { get { return EmptyBlocks == 2; } }
        protected override bool SkipPayloadToken { get { return EmptyBlocks != 0; } }
        protected override Encoding StreamEncoding { get { return Encoding.ASCII; } }
        protected override UInt64 PayloadAlignment { get { return TarBlockSize; } }
    }
}
