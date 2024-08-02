namespace ArchiveUtility
{
    public enum ArchiveUseCase
    {
        Generic,    // calculate all recipes
        DiffSource, // don't need nested contents if no reverse recipe - don't need forward recipes
        DiffTarget, // don't need nested contents if no forward recipe - don't need reverse recipes, so we can skip expensive ones
    }
}
