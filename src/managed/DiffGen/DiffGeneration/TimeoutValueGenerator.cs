/**
 * @file TimeoutValueGenerator.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
using System;
using System.IO;

namespace Microsoft.Azure.DeviceUpdate.Diffs
{
    public static class TimeoutValueGenerator
    {
        public static int GetTimeoutValue(params string[] files)
        {
            long length = 0;
            foreach (string file in files)
            {
                FileInfo fileInfo = new(file);
                length += fileInfo.Length;
            }

            return GetTimeoutValue(length);
        }

        private static int GetTimeoutValue(long fileLength)
        {
            const int ONE_MINUTE = 1000 * 60;
            const int THIRTY_SECONDS = 30 * 1000;
            const int TEN_MINUTES = ONE_MINUTE * 10;

            const int FIFTY_MEGABYTES = 50 * 1024 * 1024;

            // Let timeout be one minute per 50 megabytes of content.
            int timeout = (int)((fileLength * ONE_MINUTE) / FIFTY_MEGABYTES);

            timeout = Math.Max(timeout, THIRTY_SECONDS);
            timeout = Math.Min(timeout, TEN_MINUTES);

            return timeout;
        }
    }
}
