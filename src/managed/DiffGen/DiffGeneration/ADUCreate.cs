/**
 * @file ADUCreate.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Runtime.InteropServices;
    using System.Security.Authentication;
    using ArchiveUtility;

    class ADUCreate
    {
        public enum ArchiveItemType
        {
            blob = 0,
            chunk = 1,
            payload = 2,
        };

        public enum HashType
        {
            md5 = 0,
            sha256 = 1,
        };

        public const string ADUDIFFAPI_DLL_WINDOWS = "adudiffapi.dll";

        public static List<string> ADUDIFFAPI_DEPENDENCIES => new()
        {
#if DEBUG
            "zlibd1.dll",
            "zstdd.dll",
#else
            "zlib1.dll",
            "zstd.dll",
#endif
        };

        public const string ADUDIFFAPI_DLL = "adudiffapi";

        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr adu_diff_create_create_session();
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern void adu_diff_create_close_session(IntPtr handle);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern void adu_diff_create_set_remainder_sizes(IntPtr handle, [MarshalAs(UnmanagedType.U8)] UInt64 uncompressed, [MarshalAs(UnmanagedType.U8)] UInt64 compressed);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern void adu_diff_create_set_target_size_and_hash(IntPtr handle, [MarshalAs(UnmanagedType.U8)] UInt64 size, HashType hash_type, byte[] hash_value, IntPtr hash_value_length);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern void adu_diff_create_set_source_size_and_hash(IntPtr handle, [MarshalAs(UnmanagedType.U8)] UInt64 size, HashType hash_type, byte[] hash_value, IntPtr hash_value_length);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr adu_diff_create_add_chunk(IntPtr handle, [MarshalAs(UnmanagedType.U8)]  UInt64 offset, [MarshalAs(UnmanagedType.U8)] UInt64 length, HashType hash_type, byte[] hash_value, IntPtr hash_value_length);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr adu_diff_create_add_recipe(IntPtr handle, IntPtr item_handle, string recipe_type_name);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr adu_diff_create_add_recipe_parameter_archive_item(
            IntPtr handle, IntPtr recipe_handle,
            ArchiveItemType type, [MarshalAs(UnmanagedType.U8)] UInt64 offset, [MarshalAs(UnmanagedType.U8)] UInt64 length,
            HashType hash_type, byte[] hash_value, IntPtr hash_value_length);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int adu_diff_create_add_recipe_parameter_number(IntPtr handle, IntPtr recipe_handle, [MarshalAs(UnmanagedType.U8)]  UInt64 number);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int adu_diff_create_set_inline_asset_path(IntPtr handle, string path);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int adu_diff_create_set_remainder_path(IntPtr handle, string path);
        [DllImport(ADUDIFFAPI_DLL, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        public static extern int adu_diff_create_write(IntPtr handle, string path);

        static HashType HashAlgorithmTypeToHashType(HashAlgorithmType algorithm)
        {
            switch (algorithm)
            {
                case HashAlgorithmType.Md5: return HashType.md5;
                case HashAlgorithmType.Sha256: return HashType.sha256;
                default: throw new Exception($"Unexpected HaslAlgorithmType: {algorithm}.");
            }
        } 

        static ArchiveItemType ArchiveUtilityArchiveItemTypeToArchiveItemType(ArchiveUtility.ArchiveItemType type)
        {
            switch (type)
            {
                case ArchiveUtility.ArchiveItemType.Blob: return ArchiveItemType.blob;
                case ArchiveUtility.ArchiveItemType.Chunk: return ArchiveItemType.chunk;
                case ArchiveUtility.ArchiveItemType.Payload: return ArchiveItemType.payload;
                default: throw new Exception($"Unexpected ArchiveItemType: {type}");
            }
        }

        public static bool CanOpenSession()
        {
            try
            {
                //call adu_diff_create_create_session() and adu_diff_create_close_session(),
                //through construction and destruction
                {
                    Session session = new();
                }

                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        public class Session
        {
            ~Session()
            {
                adu_diff_create_close_session(SessionHandle);
            }

            public void SetRemainderSizes(UInt64 uncompressed, UInt64 compressed)
            {
                adu_diff_create_set_remainder_sizes(SessionHandle, uncompressed, compressed);
            }

            public void SetTargetSizeAndHash(UInt64 size, ArchiveUtility.Hash hash)
            {
                var hashType = HashAlgorithmTypeToHashType(hash.Type);
                SetTargetSizeAndHash(size, hashType, hash.Value, new IntPtr(hash.Value.Length));
            }

            void SetTargetSizeAndHash(UInt64 size, ADUCreate.HashType hash_type, byte[] hash_value, IntPtr hash_value_length)
            {
                ADUCreate.adu_diff_create_set_target_size_and_hash(SessionHandle, size, hash_type, hash_value, hash_value_length);
            }
            public void SetSourceSizeAndHash(UInt64 size, ArchiveUtility.Hash hash)
            {
                var hashType = HashAlgorithmTypeToHashType(hash.Type);
                SetSourceSizeAndHash(size, hashType, hash.Value, new IntPtr(hash.Value.Length));
            }

            void SetSourceSizeAndHash(UInt64 size, ADUCreate.HashType hash_type, byte[] hash_value, IntPtr hash_value_length)
            {
                ADUCreate.adu_diff_create_set_source_size_and_hash(SessionHandle, size, hash_type, hash_value, hash_value_length);
            }

            public ArchiveItem AddChunk(UInt64 offset, UInt64 length, ADUCreate.HashType hash_type, byte[] hash_value, IntPtr hash_value_length)
            {
                var archiveItemHandle = ADUCreate.adu_diff_create_add_chunk(SessionHandle, offset, length, hash_type, hash_value, hash_value_length);

                return new ArchiveItem(SessionHandle, archiveItemHandle);
            }

            public void AddChunk(ArchiveUtility.ArchiveItem chunk)
            {
                var offset = chunk.Offset.Value;
                var length = chunk.Length;
                var hashType = HashAlgorithmTypeToHashType(HashAlgorithmType.Sha256);
                var hashValue = HexUtility.HexStringToByteArray(chunk.Hashes[HashAlgorithmType.Sha256]);
                var hashValueLength = new IntPtr(hashValue.Length);

                var item = AddChunk(offset, length, hashType, hashValue, hashValueLength);

                item.AddRecipe(chunk.Recipes[0]);
            }

            public void SetInlineAssetPath(string path)
            {
                adu_diff_create_set_inline_asset_path(SessionHandle, path);
            }

            public void SetRemainderPath(string path)
            {
                adu_diff_create_set_remainder_path(SessionHandle, path);
            }

            public void Write(string path)
            {
                adu_diff_create_write(SessionHandle, path);
            }

            public IntPtr SessionHandle { get; private set; } = adu_diff_create_create_session();
        }

        public class ArchiveItem
        {
            public ArchiveItem(IntPtr sessionHandle, IntPtr archiveItemHandle)
            {
                SessionHandle = sessionHandle;
                ArchiveItemHandle = archiveItemHandle;
            }

            public Recipe AddRecipe(string recipeTypeName)
            {
                var recipeHandle = adu_diff_create_add_recipe(SessionHandle, ArchiveItemHandle, recipeTypeName);

                return new Recipe(SessionHandle, recipeHandle);
            }

            private static string GetNativeRecipeTypeName(RecipeType recipeType)
            {
                switch (recipeType)
                {
                    case RecipeType.Copy: return "copy_recipe";
                    case RecipeType.Region: return "region_recipe";
                    case RecipeType.Concatenation: return "concatenation_recipe";
                    case RecipeType.ApplyBsDiff: return "bsdiff_recipe";
                    case RecipeType.ApplyNestedDiff: return "nested_diff_recipe";
                    case RecipeType.Remainder: return "remainder_chunk_recipe";
                    case RecipeType.InlineAsset: return "inline_asset_recipe";
                    case RecipeType.CopySource: return "copy_source_recipe";
                    case RecipeType.ApplyZstdDelta: return "zstd_delta_recipe";
                    case RecipeType.ZstdCompression: return "zstd_compression_recipe";
                    case RecipeType.ZstdDecompression: return "zstd_decompression_recipe";
                    case RecipeType.AllZero: return "all_zero_recipe";
                    case RecipeType.GzDecompression: return "gz_decompression_recipe";
                    default:
                        throw new Exception($"Unexpected RecipeType: {recipeType}.");
                }
            }

            public void AddRecipe(ArchiveUtility.Recipe recipe)
            {
                var recipeTypeName = GetNativeRecipeTypeName(recipe.Type);

                var recipeHandle = AddRecipe(recipeTypeName);
                var parameters = recipe.GetParameters();

                foreach (var parameter in parameters)
                {
                    var paramValue = parameter.Item2;

                    if (paramValue.Type == RecipeParameterType.Number)
                    {
                        var numberParam = (NumberRecipeParameter)paramValue;
                        recipeHandle.AddNumberRecipeParameter(numberParam.Number);
                    }
                    else
                    {
                        var archiveItemParam = (ArchiveItemRecipeParameter)paramValue;
                        recipeHandle.AddArchiveItemRecipeParameter(archiveItemParam.Item);
                    }
                }
            }

            public IntPtr SessionHandle { get; private set; }
            public IntPtr ArchiveItemHandle { get; private set; }
        }

        public class Recipe
        {
            public Recipe(IntPtr sessionHandle, IntPtr recipeHandle)
            {
                SessionHandle = sessionHandle;
                RecipeHandle = recipeHandle;
            }
            public void AddNumberRecipeParameter(UInt64 number)
            {
                ADUCreate.adu_diff_create_add_recipe_parameter_number(SessionHandle, RecipeHandle, number);
            }
            public ArchiveItem AddArchiveItemRecipeParameter(ArchiveItemType type, UInt64 offset, UInt64 length, HashType hash_type, byte[] hash_value, IntPtr hash_value_length)
            {
                var handle = adu_diff_create_add_recipe_parameter_archive_item(SessionHandle, RecipeHandle, type, offset, length, hash_type, hash_value, hash_value_length);

                return new ArchiveItem(SessionHandle, handle);
            }
            public void AddArchiveItemRecipeParameter(ArchiveUtility.ArchiveItem item)
            {
                var type = ArchiveUtilityArchiveItemTypeToArchiveItemType(item.Type);

                var offset = item.Offset.GetValueOrDefault();
                var length = item.Length;
                var hashType = HashAlgorithmTypeToHashType(HashAlgorithmType.Sha256);
                var hashValue = HexUtility.HexStringToByteArray(item.Hashes[HashAlgorithmType.Sha256]);
                var hashValueLength = new IntPtr(hashValue.Length);

                var archiveItem = AddArchiveItemRecipeParameter(type, offset, length, hashType, hashValue, hashValueLength);

                if (item.Recipes.Count >= 1)
                {
                    archiveItem.AddRecipe(item.Recipes[0]);
                }
            }

            public IntPtr SessionHandle { get; private set; }
            public IntPtr RecipeHandle { get; private set; }
        }
    }
}
