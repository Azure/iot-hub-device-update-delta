namespace ArchiveUtility
{
    using System.Text.RegularExpressions;

    internal static class NamePatterns
    {
        public static string ReplaceNumbersWithWildCards(string value)
        {
            return Regex.Replace(value, @"\d+", "*");
        }
    }
}
