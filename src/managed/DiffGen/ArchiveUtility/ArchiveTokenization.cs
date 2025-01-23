/**
 * @file ArchiveTokenization.cs
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
    using System.Text.Json;
    using Microsoft.Extensions.Logging;

    using RecipeLookup = System.Collections.Generic.Dictionary<ArchiveUtility.ItemDefinition, ArchiveUtility.Recipe>;
    using SerializedPayloadList = System.Collections.Generic.List<System.Collections.Generic.KeyValuePair<ArchiveUtility.Payload, System.Collections.Generic.HashSet<ArchiveUtility.ItemDefinition>>>;
    using SerializedRecipeList = System.Collections.Generic.List<System.Collections.Generic.KeyValuePair<ArchiveUtility.ItemDefinition, System.Collections.Generic.HashSet<ArchiveUtility.Recipe>>>;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class ArchiveTokenization
    {
        public string Type { get; private set; }

        public string Subtype { get; private set; }

        public string WorkingFolder { get; set; }

        public string ItemFolder { get => Path.Combine(WorkingFolder, "items"); }

        public ItemDefinition ArchiveItem { get; set; } = null;

        // Items it is ok to take a dependency on without being present
        // in the tokenization. Used to allow diffs to depend upon
        // the source item
        public ItemDefinition SourceItem { get; set; } = null;

        public SerializedPayloadList Payload { get => PayloadCatalog.Entries.ToList(); }

        public SerializedRecipeList Recipes { get => RecipeCatalog.Entries.ToList(); }

        public RecipeLookup ForwardRecipes { get; set; } = new();

        public RecipeLookup ReverseRecipes { get; set; } = new();

        private PayloadCatalog PayloadCatalog = new();

        private RecipeCatalog RecipeCatalog = new();

        public ArchiveTokenization(string type, string subtype)
        {
            Type = type;
            Subtype = subtype;
        }

        public ArchiveTokenization(string type, string subtype, ArchiveTokenization parentTokens)
        {
            Type = type;
            Subtype = subtype;

            PayloadCatalog = parentTokens.PayloadCatalog;
            RecipeCatalog = parentTokens.RecipeCatalog;
            ForwardRecipes = parentTokens.ForwardRecipes;
            ReverseRecipes = parentTokens.ReverseRecipes;
            ArchiveItem = parentTokens.ArchiveItem;
        }

        public ArchiveTokenization CreateDiffTokens(ArchiveTokenization sourceTokens)
        {
            ArchiveTokenization diffTokens = new("Diff", "Standard");

            diffTokens.SourceItem = sourceTokens.ArchiveItem;
            diffTokens.ArchiveItem = ArchiveItem;

            foreach (Recipe recipe in ForwardRecipes.Values)
            {
                diffTokens.AddForwardRecipe(recipe);
            }

            return diffTokens;
        }

        public void SetPayload(SerializedPayloadList payloadList)
        {
            foreach (var payloadEntry in payloadList)
            {
                var payload = payloadEntry.Key;
                foreach (var item in payloadEntry.Value)
                {
                    PayloadCatalog.AddPayload(payload, item);
                }
            }
        }

        public void SetRecipes(SerializedRecipeList recipes)
        {
            foreach (var recipeEntry in recipes)
            {
                foreach (var recipe in recipeEntry.Value)
                {
                    RecipeCatalog.AddRecipe(recipe);
                }
            }
        }

        private static bool HasItemImpl(ItemDefinition item)
        {
            return item != null && item.Length > 0;
        }

        public bool HasSourceItem()
        {
            return HasItemImpl(SourceItem);
        }

        public ItemDefinition InlineAssetsItem { get; set; } = null;

        public bool HasInlineAssetsItem()
        {
            return HasItemImpl(InlineAssetsItem);
        }

        public ItemDefinition RemainderItem { get; set; } = null;

        public bool HasRemainderItem()
        {
            return HasItemImpl(RemainderItem);
        }

        private Dictionary<ItemDefinition, HashSet<string>> ItemToNamesLookup = new();
        private Dictionary<string, HashSet<ItemDefinition>> NameToItemsLookup = new();
        private Dictionary<string, HashSet<ItemDefinition>> WildcardNameToItemsLookup = new();
        private Dictionary<UInt64, HashSet<ItemDefinition>> LengthBasedItemLookup = new();
        private Dictionary<Hash, HashSet<ItemDefinition>> HashToItemLookup = new();

        public bool IsSpecialItem(ItemDefinition item)
        {
            if (item.Equals(ArchiveItem))
            {
                return true;
            }

            if (HasInlineAssetsItem() && item.Equals(InlineAssetsItem))
            {
                return true;
            }

            if (HasRemainderItem() && item.Equals(RemainderItem))
            {
                return true;
            }

            return false;
        }

        public IEnumerable<ItemDefinition> GetItemsWithSameSize(UInt64 length)
        {
            if (LengthBasedItemLookup.ContainsKey(length))
            {
                return LengthBasedItemLookup[length];
            }

            return null;
        }

        public void AddRootPayload(string name, ItemDefinition item)
        {
            Payload payload = new(ArchiveItem, name);
            PayloadCatalog.AddPayload(payload, item);
        }

        public bool HasRootPayload(string name)
        {
            Payload payload = new(ArchiveItem, name);
            return PayloadCatalog.HasPayload(payload);
        }

        public bool HasPayloadWithName(string name) => PayloadCatalog.HasPayloadWithName(name);

        public IEnumerable<ItemDefinition> GetPayloadWithName(string name) => PayloadCatalog.GetPayloadWithName(name);

        public IEnumerable<ItemDefinition> GetPayloadMatchingWildcard(string name) => PayloadCatalog.GetPayloadMatchingWildcard(name);

        public IEnumerable<ItemDefinition> GetRootPayload(string name)
        {
            Payload payload = new(ArchiveItem, name);
            return PayloadCatalog.GetPayload(payload);
        }

        public void AddRecipe(Recipe recipe)
        {
            RecipeCatalog.AddRecipe(recipe);
        }

        public void AddRecipes(IEnumerable<Recipe> recipes)
        {
            foreach (var recipe in recipes)
            {
                AddRecipe(recipe);
            }
        }

        public void AddForwardRecipe(Recipe recipe)
        {
            ForwardRecipes[recipe.Result] = recipe;
            AddRecipe(recipe);
        }

        public void AddReverseRecipe(Recipe recipe)
        {
            ReverseRecipes[recipe.Result] = recipe;
            AddRecipe(recipe);
        }

        public bool HasAnyRecipes(ItemDefinition item)
        {
            return RecipeCatalog.HasAnyRecipes(item);
        }

        public bool TryAddRecipe(Recipe recipe) => RecipeCatalog.TryAddRecipe(recipe);

        public bool HasRecipe(Recipe recipe) => RecipeCatalog.HasRecipe(recipe);

        public void ClearRecipes() => RecipeCatalog.ClearRecipes();

        public IEnumerable<Recipe> GetRemainderRecipes() => RecipeCatalog.GetRecipesUsing(RemainderItem);

        public IEnumerable<Recipe> GetInlineAssetRecipes() => RecipeCatalog.GetRecipesUsing(InlineAssetsItem);

        // Serialization
        public static JsonSerializerOptions GetStandardJsonSerializerOptions()
        {
            var options = new JsonSerializerOptions()
            {
                Converters =
                {
                    new ArchiveTokenizationJsonConverter(),
                    new ItemDefinitionJsonConverter(),
                    new HashJsonConverter(),
                    new RecipeJsonConverter(),
                },
                MaxDepth = 128,
            };

            return options;
        }

        public static ArchiveTokenization FromJsonPath(string path)
        {
            using var stream = File.OpenRead(path);
            return FromJson(stream);
        }

        public static ArchiveTokenization FromJson(Stream stream)
        {
            using var reader = new StreamReader(stream);
            var jsonText = reader.ReadToEnd();
            return FromJson(jsonText);
        }

        public static ArchiveTokenization FromJson(string jsonText)
        {
            var options = GetStandardJsonSerializerOptions();
            var deserialized = JsonSerializer.Deserialize<ArchiveTokenization>(jsonText, options);
            return deserialized;
        }

        public void WriteJson(Stream stream, bool writeIndented)
        {
            using var writer = new StreamWriter(stream, Encoding.UTF8, -1, true);
            var jsonText = ToJson(writeIndented);
            writer.Write(jsonText);
        }

        public string ToJson(bool writeIndented)
        {
            var options = GetStandardJsonSerializerOptions();
            options.WriteIndented = writeIndented;
            return JsonSerializer.Serialize(this, options);
        }

        public string ToJson()
        {
            return ToJson(false);
        }

#pragma warning disable SA1010 // Opening square brackets should be spaced correctly
        private static string[][] ArchiveExtensions = [[".ext4", "ext4"], [".ext2", "ext4"], [".ext3", "ext4"], [".tar", "tar"], [".cpio", "cpio"], [".swu", "swu"], [".zst", "zstd"], [".zstd", "zstd"], [".gz", "zip"]];
#pragma warning restore SA1010 // Opening square brackets should be spaced correctly

        public void ProcessNestedArchives(Stream stream, ArchiveUseCase useCase)
        {
            // Not useful to get detailed view of nested items if we can't reconstruct this archive from them
            if ((useCase == ArchiveUseCase.DiffTarget) && !ForwardRecipes.ContainsKey(ArchiveItem))
            {
                return;
            }

            List<Tuple<ItemDefinition, string, string>> potentialNestedArchives = new();

            foreach (var payloadEntry in PayloadCatalog.Entries)
            {
                Payload payload = payloadEntry.Key;

                if (!payload.ArchiveItem.Equals(ArchiveItem))
                {
                    continue;
                }

                var payloadName = payload.Name;

                foreach (var extInfo in ArchiveExtensions)
                {
                    var ext = extInfo[0];
                    var archiveType = extInfo[1];
                    if (payloadName.EndsWith(ext, StringComparison.OrdinalIgnoreCase))
                    {
                        var payloadItems = payloadEntry.Value;
                        foreach (var item in payloadItems)
                        {
                            potentialNestedArchives.Add(new(item, payloadName, archiveType));
                            break;
                        }
                    }
                }
            }

            using (FileFromStream file = new FileFromStream(stream, WorkingFolder))
            {
                string archivePath = file.Name;

                if (!TryExtractItems(ArchiveLoaderContext.DefaultLogger, archivePath, potentialNestedArchives.Select(n => n.Item1)))
                {
                    throw new Exception($"Couldn't extract items for nested archives for: {archivePath}");
                }
            }

            foreach (var nested in potentialNestedArchives)
            {
                var (nestedItem, payloadName, type) = nested;

                var archiveFile = nestedItem.GetExtractionPath(ItemFolder);

                using var archiveStream = File.OpenRead(archiveFile);

                ArchiveLoaderContext context = new(archiveStream, WorkingFolder, ArchiveLoaderContext.DefaultLogger, LogLevel.None)
                {
                    UseCase = useCase,
                };

                context.OriginalArchiveFileName = payloadName;

                if (ArchiveLoader.TryLoadArchive(context, out ArchiveTokenization tokens, type))
                {
                    context.Logger.LogInformation("Loaded nested archive of type: {type}", type);

                    string nestedJson = tokens.ArchiveItem.GetExtractionPath(context.WorkingFolder) + $".{type}.json";

                    context.Logger.LogInformation("Writing nested json to {NestedJson}", nestedJson);

                    using (var nestedJsonStream = File.OpenWrite(nestedJson))
                    {
                        tokens.WriteJson(nestedJsonStream, true);
                    }

                    ImportArchive(context.Logger, tokens);
                }
            }
        }

        public HashSet<ItemDefinition> GetDependencies(ItemDefinition item, bool excludeSpecialItems)
        {
            HashSet<ItemDefinition> dependencies = new();

            PopulateDependencies(item, dependencies, excludeSpecialItems);

            return dependencies;
        }

        private record Chunk(ulong Offset, ItemDefinition Item);

        private static Chunk ChunkFromSlice(Recipe recipe) => new Chunk(recipe.NumberIngredients[0], recipe.Result);

        private List<Chunk> GetChunksFromRecipes()
        {
            var slices = RecipeCatalog.GetSlicesOf(ArchiveItem);
            var chunks = slices.Select(x => ChunkFromSlice(x)).ToList();
            return chunks;
        }

        private IEnumerable<Chunk> GetAllChunks(Stream stream)
        {
            var chunks = GetChunksFromRecipes();

            // We don't want to create gap chunks if there are no already defined chunks
            if (chunks.Count == 0)
            {
                return chunks;
            }

            var sortedChunks = chunks.OrderBy(x => x.Offset);

            List<Chunk> gapChunks = new();

            ulong expectedOffset = 0;
            foreach (var chunk in sortedChunks)
            {
                if (expectedOffset < chunk.Offset)
                {
                    stream.Seek((long)expectedOffset, SeekOrigin.Begin);
                    using var reader = new BinaryReader(stream, Encoding.ASCII, true);
                    ulong length = chunk.Offset - expectedOffset;

                    var chunksForThisGap = MakeChunksForGap(reader, expectedOffset, length);
                    gapChunks.AddRange(chunksForThisGap);
                }

                expectedOffset = chunk.Offset + chunk.Item.Length;
            }

            if (expectedOffset == 0)
            {
                throw new Exception("Found no chunks of archive. Expected Offset for last chunk is zero. This would result in a chunk of entire file.");
            }

            if (expectedOffset < ArchiveItem.Length)
            {
                stream.Seek((long)expectedOffset, SeekOrigin.Begin);
                using var reader = new BinaryReader(stream, Encoding.ASCII, true);
                ulong length = ArchiveItem.Length - expectedOffset;

                var chunksForThisGap = MakeChunksForGap(reader, expectedOffset, length);
                gapChunks.AddRange(chunksForThisGap);
            }

            chunks.AddRange(gapChunks);

            return chunks.OrderBy(x => x.Offset);
        }

        private ItemDefinition CreateItemForGap(BinaryReader reader, ulong begin, ulong end)
        {
            ulong length = end - begin;

            reader.BaseStream.Seek((long)begin, SeekOrigin.Begin);
            ItemDefinition item = ItemDefinition.FromBinaryReader(reader, length).WithName(ChunkNames.MakeGapChunkName(begin, length));

            bool allZeros = true;

            ulong remaining = length;

            const int read_block_size = 1024 * 8;
            byte[] data = new byte[read_block_size];
            reader.BaseStream.Seek((long)begin, SeekOrigin.Begin);
            while (remaining > 0)
            {
                int toRead = (int)Math.Min(remaining, read_block_size);

                var span = new Span<byte>(data, 0, toRead);

                int actualRead = reader.Read(span);
                if (actualRead != toRead)
                {
                    throw new Exception($"Couldn't read gap data. Didn't read expected amount of bytes. Expected: {toRead}, Actual: {actualRead}");
                }

                if (!AsciiData.IsAllNul(span))
                {
                    allZeros = false;
                    break;
                }

                remaining -= (ulong)actualRead;
            }

            if (allZeros)
            {
                Recipe allZerosRecipe = new (Recipe.RecipeTypeToString(RecipeType.AllZeros), item, new(), new());
                AddForwardRecipe(allZerosRecipe);
            }

            Recipe sliceRecipe = new(RecipeType.Slice, item, new() { begin }, new List<ItemDefinition>() { ArchiveItem });
            AddReverseRecipe(sliceRecipe);

            return item;
        }

        private IEnumerable<Chunk> MakeChunksForGap(BinaryReader reader, ulong offset, ulong length)
        {
            var chunks = new List<Chunk>();
            ulong gapBegin = offset;
            ulong gapEnd = offset + length;

            //Align gap chunks on 1024 bytes, to make matching easier
            if (gapBegin % 1024 != 0)
            {
                ulong updatedGapBegin = ((gapBegin >> 10) + 1) << 10;

                if (updatedGapBegin <= gapEnd)
                {
                    var item = CreateItemForGap(reader, gapBegin, updatedGapBegin);
                    chunks.Add(new(gapBegin, item));
                    gapBegin = updatedGapBegin;
                }
            }

            //the maximum size allowed for gap chunks
            const ulong TEN_MB = 10 * (1 << 20);

            for (ulong thisGapBegin = gapBegin, thisGapEnd; thisGapBegin < gapEnd; thisGapBegin = thisGapEnd)
            {
                thisGapEnd = Math.Min(gapEnd, thisGapBegin + TEN_MB);

                var item = CreateItemForGap(reader, thisGapBegin, thisGapEnd);
                chunks.Add(new(gapBegin, item));
            }

            return chunks;
        }

        public void HandleGapChunks(Stream stream)
        {
            var allChunks = GetAllChunks(stream);
            var items = allChunks.Select(x => x.Item).ToList();

            // Don't overwrite whatever the json already had
            if (ForwardRecipes.ContainsKey(ArchiveItem))
            {
                return;
            }

            if (items.Count == 0)
            {
                return;
            }

            var totalItemLength = items.Sum(x => (long)x.Length);
            if (totalItemLength != (long)ArchiveItem.Length)
            {
                throw new Exception($"Total items length for chain recipe when handling gaps mismatch. Item length: {ArchiveItem.Length}, Chain total item length: {totalItemLength}");
            }

            Recipe archiveRecipe = new Recipe(RecipeType.Chain, ArchiveItem, new() { }, items);
            AddForwardRecipe(archiveRecipe);
        }

        private void PopulateDependencies(ItemDefinition item, HashSet<ItemDefinition> dependencies, bool excludeSpecialItems)
        {
            if (excludeSpecialItems && IsSpecialItem(item))
            {
                return;
            }

            if (dependencies.Contains(item))
            {
                return;
            }

            dependencies.Add(item);

            var recipes = RecipeCatalog.GetRecipes(item);

            foreach (var recipe in recipes)
            {
                foreach (var ingredient in recipe.ItemIngredients)
                {
                    PopulateDependencies(ingredient, dependencies, excludeSpecialItems);
                }
            }
        }

        public HashSet<Recipe> GetRecipes(ItemDefinition item) => RecipeCatalog.GetRecipes(item);

        public bool HasArchiveItem(ItemDefinition item) => PayloadCatalog.ArchiveItems.Contains(item);

        public IEnumerable<ItemDefinition> ArchiveItems => PayloadCatalog.ArchiveItems;

        private void ImportArchive(ILogger logger, ArchiveTokenization tokens)
        {
            var toArchive = ArchiveItem.GetSha256HashString();
            var fromArchive = tokens.ArchiveItem.GetSha256HashString();

            logger.LogInformation("Importing archive {fromArchive} into {toArchive}", fromArchive, toArchive);

            if (tokens.RecipeCatalog.HasAnyRecipes(tokens.ArchiveItem))
            {
                logger.LogInformation("Imported archive has a recipe for the ArchiveItem.");
            }
            else
            {
                logger.LogInformation("Imported archive does not have a recipe for the ArchiveItem.");
            }

            foreach (var recipeEntry in tokens.Recipes)
            {
                var result = recipeEntry.Key;
                var recipes = recipeEntry.Value;

                foreach (var recipe in recipes)
                {
                    RecipeCatalog.AddRecipe(recipe);
                }
            }

            foreach (var recipeEntry in tokens.ForwardRecipes)
            {
                var result = recipeEntry.Key;
                var recipe = recipeEntry.Value;

                ForwardRecipes[result] = recipe;
            }

            foreach (var recipeEntry in tokens.ReverseRecipes)
            {
                var result = recipeEntry.Key;
                var recipe = recipeEntry.Value;

                ReverseRecipes[result] = recipe;
            }

            foreach (var (payload, items) in tokens.Payload)
            {
                foreach (var item in items)
                {
                    PayloadCatalog.AddPayload(payload, item);
                }
            }
        }

        public bool TryExtractItems(ILogger logger, string archivePath, IEnumerable<ItemDefinition> toExtract)
        {
            try
            {
                ExtractItems(logger, archivePath, toExtract);
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        public void ExtractItems(ILogger logger, string archivePath, IEnumerable<ItemDefinition> toExtract)
        {
            using var createSession = new DiffApi.DiffcSession();
            createSession.SetTarget(ArchiveItem);

            var extractionRoot = ItemFolder;

            int recipeCount = 0;
            foreach (var entry in RecipeCatalog.Entries)
            {
                var recipes = entry.Value;
                foreach (var recipe in recipes)
                {
                    recipeCount++;
                    createSession.AddRecipe(recipe);
                }
            }

            using var applySession = createSession.NewApplySession();

            logger?.LogInformation("Trying to extract {toExtractCount:N0} items from {archivePath} using {recipeCount:N0} recipes.", toExtract.Count(), archivePath, recipeCount);

            foreach (var item in toExtract)
            {
                var itemPath = item.GetExtractionPath(extractionRoot);
                applySession.RequestItem(item);
            }

            applySession.AddItemToPantry(archivePath);

            if (!applySession.ProcessRequestedItems())
            {
                var msg = $"Can't process items from: {archivePath}";
                logger?.LogError(msg);
                throw new Exception(msg);
            }

            applySession.ResumeSlicing();

            Directory.CreateDirectory(extractionRoot);

            foreach (var item in toExtract)
            {
                var hashString = item.GetSha256HashString();
                if (hashString == null)
                {
                    var msg = "Trying to extract an item without a sha256 hash.";
                    logger?.LogError(msg);
                    throw new Exception(msg);
                }

                var itemPath = item.GetExtractionPath(extractionRoot);
                applySession.ExtractItemToPath(item, itemPath);

                var fromFile = ItemDefinition.FromFile(itemPath);
            }

            applySession.CancelSlicing();

            foreach (var item in toExtract)
            {
                var itemPath = item.GetExtractionPath(extractionRoot);
                var fromFile = ItemDefinition.FromFile(itemPath);

                if (!item.Equals(fromFile))
                {
                    var msg = $"Extracted file {itemPath} mismatch from expected result. Expected: {item}, Actual: {fromFile}";
                    logger?.LogError(msg);
                    applySession.CancelSlicing();
                    throw new Exception(msg);
                }
            }
        }
    }
}
