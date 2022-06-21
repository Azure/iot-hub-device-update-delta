/**
 * @file HexUtility.cs
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
    public class HexUtility
    {
        public static byte[] HexStringToByteArray(string hex)
        {
            return Enumerable.Range(0, hex.Length)
                             .Where(x => x % 2 == 0)
                             .Select(x => Convert.ToByte(hex.Substring(x, 2), 16))
                             .ToArray();
        }

        public static string ByteArrayToHexString(byte[] value)
        {
            if (value == null || value.Length == 0)
            {
                return null;
            }
            StringBuilder builder = new();
            foreach (byte b in value)
            {
                builder.AppendFormat("{0:X2}", b);
            }
            return builder.ToString();
        }
    }
}
