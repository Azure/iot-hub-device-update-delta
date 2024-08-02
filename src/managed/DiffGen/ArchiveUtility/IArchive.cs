/**
 * @file IArchive.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
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
