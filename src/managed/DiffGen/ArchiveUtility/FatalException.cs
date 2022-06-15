/**
 * @file FatalException.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
﻿namespace ArchiveUtility
{
    using System;

    public class FatalException : Exception
    {
        public FatalException(string msg) : base(msg)
        {
        }
    }
}
