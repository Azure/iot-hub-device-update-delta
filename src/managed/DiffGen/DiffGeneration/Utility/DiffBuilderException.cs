/**
 * @file DiffBuilderException.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace Microsoft.Azure.DeviceUpdate.Diffs.Utility;

using System;

public enum FailureType
{
    Generic,
    SourcePackageInvalid,
    TargetPackageInvalid,
    DiffGeneration,
    BadWorkingFolder,
    UndiffableTarget,
}

public class DiffBuilderException : Exception
{
    public DiffBuilder DiffBuilder { get; private set; }

    public bool Retryable { get; private set; }

    public FailureType FailureType { get; private set; }

    public DiffBuilderException(string msg, DiffBuilder diffBuilder)
        : this(msg, diffBuilder, false)
    {
    }

    public DiffBuilderException(string msg, DiffBuilder diffBuilder, bool retryable)
        : base(msg)
    {
        DiffBuilder = diffBuilder;
        Retryable = retryable;
    }

    public DiffBuilderException(string msg, FailureType failureType)
        : base(msg)
    {
        FailureType = failureType;
    }
}
