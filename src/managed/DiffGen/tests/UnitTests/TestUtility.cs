/**
 * @file TestUtility.cs
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
namespace UnitTests
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.IO;
    using System.Text;

    class TestUtility
    {
        public record LineDifference(string Expected, string Actual, int LineNumber)
        {
            public override string ToString()
            {
                return $"{LineNumber}) Expected: \"{Expected}\", Actual: \"{Actual}\"";
            }
        }

        public static List<LineDifference> GetLineDifferences(string expected, string actual)
        {
            using StringReader expectedReader = new StringReader(expected);
            using StringReader actualReader = new StringReader(actual);
            int lineNumber = 0;

            List<LineDifference> differences = new();

            while (true)
            {
                string expectedLine = expectedReader.ReadLine();
                string actualLine = actualReader.ReadLine();

                lineNumber++;

                if (actualLine == null && expectedLine == null)
                {
                    break;
                }

                if (expectedLine == null)
                {
                    differences.Add(new(string.Empty, actualLine, lineNumber));
                    continue;
                }
                else if (actualLine == null)
                {
                    differences.Add(new(expectedLine, string.Empty, lineNumber));
                    continue;
                }


                if (expectedLine.Equals(actualLine))
                {
                    continue;
                }

                differences.Add(new (expectedLine, actualLine, lineNumber));
            }

            return differences;
        }

        static string GetStringDifferences(string expected, string actual)
        {
            var differences = GetLineDifferences(expected, actual);

            StringBuilder differentLinesStringBuilder = new();
            foreach (var diff in differences)
            {
                differentLinesStringBuilder.AppendLine(diff.ToString());
            }

            return differentLinesStringBuilder.ToString();
        }

        public static void CompareMultilineStrings(string expected, string actual)
        {
            var differences = GetStringDifferences(expected, actual);

            Assert.AreEqual("", differences);
        }
        public static void CompareMultilineStrings(string expected, string actual, string message)
        {
            var differences = GetStringDifferences(expected, actual);

            Assert.AreEqual("", differences, message);
        }
    }
}