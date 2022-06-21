/**
 * @file IArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ArchiveUtility
{
    public interface IArchive
    {
        public bool IsMatchingFormat();
        public ArchiveTokenization Tokenize();
        public string ArchiveType { get; }
        public string ArchiveSubtype { get; }
    }
}
