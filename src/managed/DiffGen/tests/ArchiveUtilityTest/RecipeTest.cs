/**
 * @file RecipeTest.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace ArchiveUtilityTest
{
    using System;
    using System.Linq;

    using ArchiveUtility;

    [TestClass]
    public class RecipeTest
    {
        [TestMethod]
        public void TestRecipeExport_Simple()
        {
            byte[] data = new byte[10] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

            var hashes = BinaryData.CalculateHashes(data);

            ArchiveItem chunk = ArchiveItem.CreateChunk("TestChunk", 1000, 2000, hashes);
            ArchiveItem payload = ArchiveItem.CreatePayload("TestPayload", 2000, hashes);

            Recipe chunkRecipe = new CopyRecipe(chunk);

            Recipe exportedChunkRecipe = chunkRecipe.Export();

            Assert.AreEqual(RecipeType.CopySource, exportedChunkRecipe.Type);
            Assert.AreEqual(1, exportedChunkRecipe.GetParameters().ToArray().Count());

            var copySourceRecipe = exportedChunkRecipe as CopySourceRecipe;
            Assert.IsNotNull(copySourceRecipe);
            Assert.AreEqual(copySourceRecipe.Offset, chunk.Offset);

            Recipe payloadRecipe = new CopyRecipe(payload);

            Recipe exportedPaylodRecipe = payloadRecipe.Export();
            Assert.AreEqual(RecipeType.Copy, exportedPaylodRecipe.Type);
            Assert.AreEqual(1, exportedPaylodRecipe.GetParameters().ToArray().Count());

            ArchiveItem badChunk = new ArchiveItem("BadChunk", ArchiveItemType.Chunk, null, 2000, null, hashes);
            Recipe badRecipe = new CopyRecipe(badChunk);

            bool caughtException = false;
            try
            {
                Recipe badExportedRecipe = badRecipe.Export();
            }
            catch (Exception)
            {
                caughtException = true;
            }

            Assert.IsTrue(caughtException);
        }

        [TestMethod]
        public void TestRecipeExport_RegionOfChunk()
        {
            byte[] data = new byte[10] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

            var hashes = BinaryData.CalculateHashes(data);

            var chunk = ArchiveItem.CreateChunk("TestChunk", 1000, 2000, hashes);

            var regionOfChunkRecipe = new RegionRecipe(chunk, 100, 500);

            var exportedRegionOfChunkRecipe = regionOfChunkRecipe.Export() as RegionRecipe;
            Assert.IsNotNull(exportedRegionOfChunkRecipe);

            Assert.AreEqual(RecipeType.Region, exportedRegionOfChunkRecipe.Type);
            Assert.AreEqual(3, exportedRegionOfChunkRecipe.GetParameters().ToArray().Count());

            Assert.AreEqual(1, exportedRegionOfChunkRecipe.Item.Recipes.Count);
            Assert.AreEqual(ArchiveItemType.Blob, exportedRegionOfChunkRecipe.Item.Type);
            Assert.AreEqual(RecipeType.CopySource, exportedRegionOfChunkRecipe.Item.Recipes[0].Type);

            var copySourceRecipe = exportedRegionOfChunkRecipe.Item.Recipes[0] as CopySourceRecipe;
            Assert.IsNotNull(copySourceRecipe);
            Assert.AreEqual(1000u, copySourceRecipe.Offset);
        }

        [TestMethod]
        public void TestRecipeExport_RegionOfCopyChunk()
        {
            byte[] data = new byte[10] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

            var hashes = BinaryData.CalculateHashes(data);

            var chunk = ArchiveItem.CreateChunk("TestChunk", 1000, 2000, hashes);

            var blob = ArchiveItem.CreateBlob(chunk.Name, chunk.Length, hashes);
            blob.Recipes.Add(new CopyRecipe(chunk));
            Recipe regionOfCopyChunkRecipe = new RegionRecipe(blob, 100, 500);

            var exportedRecipe = regionOfCopyChunkRecipe.Export();
            Assert.AreEqual(RecipeType.Region, exportedRecipe.Type);
            var exportedRegionOfCopyChunkRecipe = exportedRecipe as RegionRecipe;
            Assert.IsNotNull(exportedRegionOfCopyChunkRecipe);

            Assert.AreEqual(RecipeType.Region, exportedRegionOfCopyChunkRecipe.Type);
            Assert.AreEqual(3, exportedRegionOfCopyChunkRecipe.GetParameters().ToArray().Count());

            Assert.AreEqual(1, exportedRegionOfCopyChunkRecipe.Item.Recipes.Count);
            Assert.AreEqual(ArchiveItemType.Blob, exportedRegionOfCopyChunkRecipe.Item.Type);
            Assert.AreEqual(RecipeType.CopySource, exportedRegionOfCopyChunkRecipe.Item.Recipes[0].Type);

            var otherCopySourceRecipe = exportedRegionOfCopyChunkRecipe.Item.Recipes[0] as CopySourceRecipe;
            Assert.IsNotNull(otherCopySourceRecipe);
            Assert.AreEqual(1000u, otherCopySourceRecipe.Offset);
        }

        void VerifyConcatParam(Recipe recipe, string name)
        {
            var itemParam = recipe.GetParameter(name) as ArchiveItemRecipeParameter;
            Assert.IsNotNull(itemParam);
            Assert.AreEqual(1, itemParam.Item.Recipes.Count);
            Assert.AreEqual(RecipeType.CopySource, itemParam.Item.Recipes[0].Type);
        }

        [TestMethod]
        public void TestRecipeExport_RegionOfConcatenation()
        {
            byte[] data0 = new byte[10] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            byte[] data1 = new byte[10] { 1, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            byte[] data2 = new byte[10] { 2, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            byte[] data3 = new byte[10] { 2, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

            var hashes0 = BinaryData.CalculateHashes(data0);
            var hashes1 = BinaryData.CalculateHashes(data1);
            var hashes2 = BinaryData.CalculateHashes(data2);
            var hashes3 = BinaryData.CalculateHashes(data3);

            var chunk0 = ArchiveItem.CreateChunk("TestChunk0", 1000, 1000, hashes0);
            var chunk1 = ArchiveItem.CreateChunk("TestChunk1", 5000, 2000, hashes1);
            var chunk2 = ArchiveItem.CreateChunk("TestChunk2", 10000, 3000, hashes2);

            var blob = ArchiveItem.CreateBlob("Concatenated", 6000, hashes3);
            blob.Recipes.Add(new ConcatenationRecipe(chunk0, chunk1, chunk2));

            Recipe regionRecipe = new RegionRecipe(blob, 500, 3000);
            var exportedRecipe = regionRecipe.Export();
            Assert.AreEqual(RecipeType.Region, exportedRecipe.Type);
            var exportedRegionRecipe = exportedRecipe as RegionRecipe;
            Assert.IsNotNull(exportedRegionRecipe);

            Assert.AreEqual(RecipeType.Region, exportedRegionRecipe.Type);
            Assert.AreEqual(3, exportedRegionRecipe.GetParameters().ToArray().Count());

            Assert.AreEqual(1, exportedRegionRecipe.Item.Recipes.Count);
            Assert.AreEqual(ArchiveItemType.Blob, exportedRegionRecipe.Item.Type);

            Assert.AreEqual(1, exportedRegionRecipe.Item.Recipes.Count);
            var concatRecipe = exportedRegionRecipe.Item.Recipes[0];
            Assert.AreEqual(RecipeType.Concatenation, concatRecipe.Type);

            Assert.AreEqual(3, concatRecipe.GetParameters().ToArray().Count());
            VerifyConcatParam(concatRecipe, "0");
            VerifyConcatParam(concatRecipe, "1");
            VerifyConcatParam(concatRecipe, "2");
        }

        [TestMethod]
        public void GetDependencies()
        {

        }
    }
}