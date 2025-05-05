/**
 * @file ArchiveLoader.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading;

    using Microsoft.Extensions.Logging;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
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

        public static bool TryLoadArchive(ArchiveLoaderContext context, out ArchiveTokenization tokens, IArchive archive)
        {
            var type = archive.GetType();

            context.Logger?.Log(context.LogLevel, "Testing if archive is format: {0}", type.FullName);

            if (context.TokenCache.ContainsKey(type))
            {
                tokens = context.TokenCache[type];
                context.Logger?.Log(context.LogLevel, "Found previously loaded tokens for type: {0}. ArchiveItem: {1}", type.FullName, tokens.ArchiveItem.GetSha256HashString());

                return true;
            }

            if (!archive.IsMatchingFormat())
            {
                context.Logger?.Log(context.LogLevel, "Archive is not of format: {0}", type.FullName);
                tokens = null;
                return false;
            }
            else
            {
                context.Logger?.Log(context.LogLevel, "Archive is of format: {0}", type.FullName);
            }

            // We may have already gotten tokens from determing if this was the same type
            if (context.TokenCache.ContainsKey(type))
            {
                tokens = context.TokenCache[type];
                return true;
            }

            if (context.CancellationToken != CancellationToken.None)
            {
                context.CancellationToken.ThrowIfCancellationRequested();
            }

            try
            {
                tokens = archive.Tokenize();

                context.TokenCache[type] = tokens;
                context.Logger?.Log(context.LogLevel, "Setting into Token Cache: ArchiveItem: {0}", tokens.ArchiveItem.GetSha256HashString());

                return true;
            }
            catch (FatalException)
            {
                throw;
            }
            catch (Exception e)
            {
                context.Logger?.LogError("Exception occurred while processing format: {0}", type.FullName);
                context.Logger?.LogError("Exception: {0}", e.ToString());
                tokens = null;
                return false;
            }
        }

        public static bool TryLoadArchive(ArchiveLoaderContext context, out ArchiveTokenization tokens, params string[] typesToTry)
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

                    if (TryLoadArchive(context, out tokens, archive))
                    {
                        return true;
                    }
                }
            }

            tokens = null;
            return false;
        }
    }
}
