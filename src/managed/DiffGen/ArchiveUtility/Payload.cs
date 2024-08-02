namespace ArchiveUtility
{
    public record Payload(ItemDefinition ArchiveItem, string Name)
    {
        public static string GetWildcardPayloadName(string name)
        {
            string fileName = GetPayloadFileName(name);

            if (fileName.Length == name.Length)
            {
                return fileName;
            }

            int pathLength = name.Length - fileName.Length;
            string wildcardPath = NamePatterns.ReplaceNumbersWithWildCards(name.Substring(0, pathLength));

            return wildcardPath + fileName;
        }

        public static string GetPayloadFileName(string name)
        {
            int index = name.LastIndexOf('/');

            if (index == -1)
            {
                return name;
            }

            return name.Substring(index + 1);
        }

        public string GetWildcardPayloadName()
        {
            return GetWildcardPayloadName(Name);
        }
    }
}
