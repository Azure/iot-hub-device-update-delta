/**
 * @file DiffApi.cs
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
    using System.Runtime.InteropServices;
    using System.Security.Authentication;

    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121", Justification = "We want to be explicit about bit-width using these aliases.")]
    public class DiffApi
    {
        public enum HashType
        {
            md5 = 0,
            sha256 = 1,
        }

        private static HashType HashAlgorithmTypeToHashType(HashAlgorithmType algorithm)
        {
            switch (algorithm)
            {
                case HashAlgorithmType.Md5: return HashType.md5;
                case HashAlgorithmType.Sha256: return HashType.sha256;
                default: throw new Exception($"Unexpected HaslAlgorithmType: {algorithm}.");
            }
        }

        public class PinnedBuffer : IDisposable
        {
            public byte[] Buffer { get; init; }

            public GCHandle Handle { get; init; }

            public PinnedBuffer(byte[] buffer)
            {
                Buffer = buffer;
                Handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            }

            public void Dispose()
            {
                Handle.Free();
            }
        }

        public class PinnedHash : IDisposable
        {
            public NativeObjects.Hash Value { get; init; }

            public GCHandle Handle { get; init; }

            public PinnedHash(HashType hashType, IntPtr hashValue, UIntPtr hashLength)
            {
                Value = new NativeObjects.Hash(hashType, hashValue, hashLength);
                Handle = GCHandle.Alloc(Value, GCHandleType.Pinned);
            }

            public void Dispose()
            {
                Handle.Free();
            }
        }

        public class ItemDefinitionObject : IDisposable
        {
            private Dictionary<HashType, PinnedBuffer> _hashBytes = new();
            private Dictionary<HashType, PinnedHash> _hashStructures = new();
            private IntPtr[] _hashesArray;
            private GCHandle _hashesHandle;
            private IntPtr[] _namesArray;
            private GCHandle _namesHandle;

            public NativeObjects.ItemDefinition Structure { get; init; }

            public IntPtr Pointer { get; init; }

            public ItemDefinitionObject(ArchiveUtility.ItemDefinition item)
            {
                _hashesArray = new IntPtr[item.Hashes.Count];
                for (int hashIndex = 0; hashIndex < item.Hashes.Count; hashIndex++)
                {
                    var entry = item.Hashes.ElementAt(hashIndex);
                    var hash = entry.Value;
                    var hashType = HashAlgorithmTypeToHashType(hash.Type);

                    PinnedBuffer pinnedBuffer = new(hash.Value.ToArray());
                    _hashBytes.Add(hashType, pinnedBuffer);

                    var hashValue = pinnedBuffer.Handle.AddrOfPinnedObject();
                    var hashLength = (UIntPtr)hash.Value.Length;
                    PinnedHash pinnedHash = new(hashType, hashValue, hashLength);
                    _hashStructures.Add(hashType, pinnedHash);

                    _hashesArray[hashIndex] = pinnedHash.Handle.AddrOfPinnedObject();
                }

                _hashesHandle = GCHandle.Alloc(_hashesArray, GCHandleType.Pinned);

                _namesArray = new IntPtr[item.Names.Count];
                for (int nameIndex = 0; nameIndex < item.Names.Count; nameIndex++)
                {
                    var name = item.Names[nameIndex];
                    var namePtr = Marshal.StringToHGlobalAnsi(name);
                    _namesArray[nameIndex] = namePtr;
                }

                _namesHandle = GCHandle.Alloc(_namesArray, GCHandleType.Pinned);

                var length = item.Length;
                IntPtr hashesPtr = _hashesHandle.AddrOfPinnedObject();
                UIntPtr hashesLength = (UIntPtr)item.Hashes.Count;
                IntPtr namesPtr = _namesHandle.AddrOfPinnedObject();
                UIntPtr namesLength = (UIntPtr)item.Names.Count;

                Structure = new(length, hashesPtr, hashesLength, namesPtr, namesLength);

                Pointer = Marshal.AllocHGlobal(Marshal.SizeOf(Structure));
                Marshal.StructureToPtr(Structure, Pointer, false);
            }

            public void Dispose()
            {
                foreach (var entry in _hashBytes)
                {
                    entry.Value.Dispose();
                }

                foreach (var entry in _hashStructures)
                {
                    entry.Value.Dispose();
                }

                _hashesHandle.Free();

                foreach (IntPtr namePtr in _namesArray)
                {
                    Marshal.FreeCoTaskMem(namePtr);
                }

                _namesHandle.Free();

                Marshal.FreeHGlobal(Pointer);
            }
        }

        public class NumberIngredients : IDisposable
        {
            private UInt64[] _values;
            private GCHandle _pinned;

            public IntPtr Array { get; init; }

            public UIntPtr Count { get; init; }

            public NumberIngredients()
                : this(new UInt64[0])
            {
            }

            public NumberIngredients(IEnumerable<UInt64> values)
            {
                _values = values.ToArray();
                _pinned = GCHandle.Alloc(_values, GCHandleType.Pinned);

                Array = _pinned.AddrOfPinnedObject();
                Count = (UIntPtr)_values.Length;
            }

            public void Dispose()
            {
                _pinned.Free();
            }
        }

        public class ItemIngredients : IDisposable
        {
            private ItemDefinitionObject[] _objects;
            private IntPtr[] _array;
            private GCHandle _pinned;

            public IntPtr Array { get; init; }

            public UIntPtr Count { get; init; }

            public ItemIngredients(IEnumerable<ItemDefinition> items)
            {
                _objects = items.Select(x => new ItemDefinitionObject(x)).ToArray();
                _array = _objects.Select(x => x.Pointer).ToArray();
                _pinned = GCHandle.Alloc(_array, GCHandleType.Pinned);

                Array = _pinned.AddrOfPinnedObject();
                Count = (UIntPtr)_array.Length;
            }

            public void Dispose()
            {
                _pinned.Free();

                foreach (var obj in _objects)
                {
                    obj.Dispose();
                }
            }
        }

        public static string GetNativePath(string path)
        {
            if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                return path;
            }

            var nativePath = Path.GetFullPath(path);

            if (nativePath[1] != ':')
            {
                return nativePath;
            }

            nativePath = @"\\?\" + nativePath;

            return nativePath;
        }

        public class DiffaSession : IDisposable
        {
            private IntPtr _session;

            public DiffaSession()
            {
                _session = NativeMethods.diffa_open_session();
            }

            public DiffaSession(IntPtr session)
            {
                _session = session;
            }

            public void Dispose()
            {
                NativeMethods.diffa_close_session(_session);
            }

            public void AddArchive(string path)
            {
                var nativePath = GetNativePath(path);
                var ret = NativeMethods.diffa_add_archive(_session, nativePath);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffa_add_archive() failed with ret: {ret}");
                }
            }

            public void RequestItem(ItemDefinition item)
            {
                using ItemDefinitionObject nativeItem = new(item);
                var ret = NativeMethods.diffa_request_item(_session, nativeItem.Pointer);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffa_request_item() failed with ret: {ret}");
                }
            }

            public void AddItemToPantry(string path)
            {
                var nativePath = GetNativePath(path);
                var ret = NativeMethods.diffa_add_file_to_pantry(_session, nativePath);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffa_add_file_to_pantry() failed with ret: {ret}");
                }
            }

            public void ClearRequestedItems()
            {
                var ret = NativeMethods.diffa_clear_requested_items(_session);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffa_clear_requested_items() failed with ret: {ret}");
                }
            }

            public bool ProcessRequestedItems()
            {
                return NativeMethods.diffa_process_requested_items(_session) == 0;
            }

            public void ResumeSlicing()
            {
                var ret = NativeMethods.diffa_resume_slicing(_session);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffa_resume_slicing() failed with ret: {ret}");
                }
            }

            public void CancelSlicing()
            {
                var ret = NativeMethods.diffa_cancel_slicing(_session);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffa_cancel_slicing() failed with ret: {ret}");
                }
            }

            public void ExtractItemToPath(ItemDefinition item, string path)
            {
                using ItemDefinitionObject nativeItem = new(item);
                var nativePath = GetNativePath(path);
                var ret = NativeMethods.diffa_extract_item_to_path(_session, nativeItem.Pointer, nativePath);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffa_extract_item_to_path() failed with ret: {ret}");
                }
            }
        }

        public class DiffcSession : IDisposable
        {
            private IntPtr _session;

            public DiffcSession()
            {
                _session = NativeMethods.diffc_open_session();
            }

            public void Dispose()
            {
                NativeMethods.diffc_close_session(_session);
            }

            public void SetTarget(ArchiveUtility.ItemDefinition item)
            {
                using ItemDefinitionObject nativeItem = new(item);

                var ret = NativeMethods.diffc_set_target(_session, nativeItem.Pointer);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffc_set_target() failed with ret: {ret}");
                }
            }

            public void SetSource(ArchiveUtility.ItemDefinition item)
            {
                using ItemDefinitionObject nativeItem = new(item);

                var ret = NativeMethods.diffc_set_source(_session, nativeItem.Pointer);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffc_set_source() failed with ret: {ret}");
                }
            }

            public void AddRecipe(ArchiveUtility.Recipe recipe)
            {
                using ItemDefinitionObject resultItem = new(recipe.Result);
                using NumberIngredients numbers = new(recipe.NumberIngredients);
                using ItemIngredients items = new(recipe.ItemIngredients);

                var ret = NativeMethods.diffc_add_recipe(_session, recipe.Name, resultItem.Pointer, numbers.Array, numbers.Count, items.Array, items.Count);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffc_add_recipe() failed with ret: {ret}");
                }
            }

            public void SetInlineAssets(string path)
            {
                var nativePath = GetNativePath(path);
                var ret = NativeMethods.diffc_set_inline_assets(_session, nativePath);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffc_set_inline_assets() failed with ret: {ret}");
                }
            }

            public void SetRemainder(string path)
            {
                var nativePath = GetNativePath(path);
                var ret = NativeMethods.diffc_set_remainder(_session, nativePath);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffc_set_remainder() failed with ret: {ret}");
                }
            }

            public void AddNestedArchive(string path)
            {
                var nativePath = GetNativePath(path);
                var ret = NativeMethods.diffc_add_nested_archive(_session, nativePath);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffc_add_nested_archive() failed with ret: {ret}");
                }
            }

            public void WriteDiff(string path)
            {
                var nativePath = GetNativePath(path);
                var ret = NativeMethods.diffc_write_diff(_session, nativePath);
                if (ret != 0)
                {
                    throw new Exception($"NativeMethods.diffc_write_diff() failed with ret: {ret}");
                }
            }

            public DiffaSession NewApplySession()
            {
                var applyHandle = NativeMethods.diffc_new_diffa_session(_session);

                return new DiffaSession(applyHandle);
            }
        }

        public static bool CanOpenSession()
        {
            try
            {
                //call diffc_open_session() and diffc_close_session(),
                //through construction and destruction
                {
                    DiffcSession session = new();
                }

                return true;
            }
            catch (Exception e)
            {
                Console.WriteLine("Exception while trying to open session: {0}", e.Message);
                return false;
            }
        }

        public class NativeObjects
        {
            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
            public struct Hash
            {
                public Hash(HashType hashType, IntPtr hashValue, UIntPtr hashLength)
                {
                    this.hashType = (UInt32)hashType;
                    this.hashValue = hashValue;
                    this.hashLength = (UIntPtr)hashLength;
                }

                public UInt32 hashType;
                public IntPtr hashValue;
                public UIntPtr hashLength;
            }

            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
            public struct ItemDefinition
            {
                public ItemDefinition(UInt64 length, IntPtr hashes, UIntPtr hashesLength, IntPtr names, UIntPtr namesLength)
                {
                    this.length = length;
                    this.hashes = hashes;
                    this.hashesLength = hashesLength;
                    this.names = names;
                    this.namesLength = namesLength;
                }

                public UInt64 length;
                public IntPtr hashes;
                public UIntPtr hashesLength;
                public IntPtr names;
                public UIntPtr namesLength;
            }
        }

        public const string AduDiffApiDll = "adudiffapi";
        public const string AduDiffApiDllWindows = "adudiffapi.dll";

        public static List<string> AduDiffApiWindowsDependencies => new()
        {
#if DEBUG
            "zlibd1.dll",
            "zstdd.dll",
#else
            "zlib1.dll",
            "zstd.dll",
#endif
        };

        public class NativeMethods
        {
            /* Create APIs */

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr diffc_open_session();

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern void diffc_close_session(IntPtr handle);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_set_target(IntPtr handle, IntPtr item);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_set_source(IntPtr handle, IntPtr item);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_add_recipe(
                IntPtr handle,
                [MarshalAs(UnmanagedType.LPStr)] string recipe_name,
                IntPtr item,
                IntPtr number_ingredients,
                UIntPtr number_ingredient_count,
                IntPtr item_ingredients,
                UIntPtr item_ingredient_count);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_set_inline_assets(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string path);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_set_remainder(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string path);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_add_nested_archive(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string path);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_add_payload(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string name, NativeObjects.ItemDefinition item);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_finalize_legacy_recipes(IntPtr handle);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_write_diff(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string path);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr diffc_new_diffa_session(IntPtr handle);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffc_get_error_code(IntPtr handle, [MarshalAs(UnmanagedType.U4)] int index);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern string diffc_get_error_text(IntPtr handle, [MarshalAs(UnmanagedType.U4)] int index);

            /* Apply APIs */

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr diffa_open_session();

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern void diffa_close_session(IntPtr handle);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_add_archive(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string path);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_request_item(IntPtr handle, IntPtr item);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_add_file_to_pantry(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string path);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_clear_requested_items(IntPtr handle);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_process_requested_items(IntPtr handle);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_resume_slicing(IntPtr handle);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_cancel_slicing(IntPtr handle);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_extract_item_to_path(IntPtr handle, IntPtr item, [MarshalAs(UnmanagedType.LPStr)] string path);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern uint diffa_get_error_code(IntPtr handle, [MarshalAs(UnmanagedType.U4)] int index);

            [DllImport(AduDiffApiDll, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
            public static extern string diffa_get_error_text(IntPtr handle, [MarshalAs(UnmanagedType.U4)] int index);
        }
    }
}