/**
 * @file AsciiData.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace ArchiveUtility
{
    using System;
    using System.Linq;
    using System.Text;

    public class AsciiData
    {
        const byte NUL = 0;
        const byte SPACE = 0x32;

        public static bool IsAllNul(byte[] bytes)
        {
            return bytes.All(b => b == NUL);
        }

        public static UInt64 FromOctalData(byte[] bytes, int offset, int length)
        {
            return FromOctalData(bytes, offset, length, true);
        }

        private static bool IsTrailingCharacter(byte b)
        {
            return b == NUL || b == SPACE;
        }
        public static UInt64 FromOctalData(byte[] bytes, int offset, int length, bool detectTrailingCharacters)
        {
            UInt64 value = 0;

            if (detectTrailingCharacters)
            {
                for (; length >= 1; length--)
                {
                    var lastByte = bytes[offset + length - 1];
                    if (!IsTrailingCharacter(lastByte))
                    {
                        break;
                    }
                }
            }

            if (length == 0)
            {
                return 0;
            }

            for (int i = 0; i < length; i++)
            {
                var span = new ReadOnlySpan<byte>(bytes, (int) (offset + i), 1);
                var octalDigit = Encoding.ASCII.GetString(span);
                var significance = (UInt64)1 << ((length - (i + 1)) * 3);
                var digitValue = Convert.ToUInt32(octalDigit, 8);
                value += digitValue * significance;
            }

            return value;
        }

        public static UInt64 FromHexadecimalData(byte[] bytes, int offset, int length)
        {
            UInt64 value = 0;

            int hexDigitCount = length / 2;

            if (length % 2 != 0)
            {
                var span = new ReadOnlySpan<byte>(bytes, 0, 1);
                var hexDigit = Encoding.ASCII.GetString(span);
                var significance = (UInt64)1 << hexDigitCount;
                var digitValue = Convert.ToUInt64(hexDigit, 16);
                value += digitValue * significance;
            }

            for (int i = 0; i < hexDigitCount; i++)
            {
                var span = new ReadOnlySpan<byte>(bytes, offset + (i * 2), 2);
                var hexDigit = Encoding.ASCII.GetString(span);
                var significance = (UInt64)1 << ((hexDigitCount - (i + 1)) * 8);
                var digitValue = Convert.ToUInt64(hexDigit, 16);
                value += digitValue * significance;
            }

            return value;
        }

        public static string FromNulPaddedString(byte[] bytes, int offset, int length)
        {
            var span = new ReadOnlySpan<byte>(bytes, offset, length);

            string value = Encoding.ASCII.GetString(span);
            while (value.EndsWith('\0'))
            {
                value = value.Remove(value.Length - 1);
            }

            return value;
        }
    }
}
