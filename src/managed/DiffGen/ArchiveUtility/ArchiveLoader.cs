/**
 * @file ArchiveLoader.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Threading;
using Microsoft.Extensions.Logging;

namespace ArchiveUtility
{
    public class ArchiveLoaderContext
    {
        public Stream Stream;
        public string WorkingFolder;
        public Dictionary<Type, ArchiveTokenization> TokenCache = new();
        public ILogger Logger;
        public CancellationToken CancellationToken = CancellationToken.None;

        public ArchiveLoaderContext(Stream stream, string workingFolder)
        {
            Stream = stream;
            WorkingFolder = workingFolder;
        }
    }

    public class ArchiveLoader
    {
        public static void RegisterArchiveType(Type type, int priority)
        {
            if (!typeof(IArchive).IsAssignableFrom(type))
            {
                throw new Exception($"ArchiveType entry {type.Name} does not implement IArchive");
            }

            if (!ArchiveTypesByPriority.ContainsKey(priority))
            {
                ArchiveTypesByPriority[priority] = new SortedDictionary<string, Type>();
            }

            var typesWithPriorityByName = ArchiveTypesByPriority[priority];
            if (!typesWithPriorityByName.ContainsKey(type.Name))
            {
                typesWithPriorityByName.Add(type.Name, type);
            }
        }
        private static SortedDictionary<int, SortedDictionary<string, Type>> ArchiveTypesByPriority = new();

        public static void LoadArchive(Stream stream, string workingFolder, out ArchiveTokenization tokens, params string[] typesToTry)
        {
            LoadArchive(new ArchiveLoaderContext(stream, workingFolder), out tokens, typesToTry);
        }

        public static void LoadArchive(ArchiveLoaderContext context, out ArchiveTokenization tokens, params string[] typesToTry)
        {
            foreach (var typesWithPriorityByName in ArchiveTypesByPriority.Values)
            {
                foreach (Type type in typesWithPriorityByName.Values)
                {
                    if (context.CancellationToken != CancellationToken.None)
                    {
                        context.CancellationToken.ThrowIfCancellationRequested();
                    }

                    var ctor = type.GetConstructor(new[] { typeof(ArchiveLoaderContext) });
                    var archive = (IArchive)ctor.Invoke(new object[] { context });

                    // Allows for selective loading of classes of archives only - this way we can look
                    // for a basic type while trying to load a composite type such as a SWUpdate file which 
                    // is a cpio archive containing a specific layout.
                    // We can call ArchiveLoader(stream, "cpio", out tokens) in the SWUpdate Archive
                    // implementation to determine if a given stream is a cpio archive and then use
                    // that tokenization to determine if the layout matches.
                    if (typesToTry.Length != 0 && typesToTry.All(t => !t.Equals(archive.ArchiveType)))
                    {
                        continue;
                    }

                    if (archive.IsMatchingFormat())
                    {
                        if (context.CancellationToken != CancellationToken.None)
                        {
                            context.CancellationToken.ThrowIfCancellationRequested();
                        }

                        try
                        {
                            tokens = archive.Tokenize();

                            //go through the archive again, to fill any gaps
                            context.Stream.Seek(0, SeekOrigin.Begin);
                            tokens.FillChunkGaps(context.Stream);

                            return;
                        }
                        catch (FatalException)
                        {
                            throw;
                        }
                        catch (Exception)
                        {
                            continue;
                        }
                    }
                }
            }

            throw new FormatException($"Couldn't load any format from stream.");
        }

        public static bool TryLoadArchive(Stream stream, string workingFolder, out ArchiveTokenization tokens, params string[] typesToTry)
        {
            return TryLoadArchive(new ArchiveLoaderContext(stream, workingFolder), out tokens, typesToTry);
        }

        public static bool TryLoadArchive(ArchiveLoaderContext context, out ArchiveTokenization tokens, params string[] typesToTry)
        {
            try
            {
                LoadArchive(context, out tokens, typesToTry);
                return true;
            }
            catch (FormatException)
            {
            }

            tokens = null;
            return false;
        }
    }
}
