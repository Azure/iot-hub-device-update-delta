There are a few classes of exceptions that canbe thrown during Diff Generation.

Most of these are fatal, though various IOExceptions may be retryable.
Our code uses a DiffBuilderException type when it detects an expected type of failure. There are other exceptions used as well, which are 
generally fatal.

Here are a list of the places we throw DiffBuilderException and their meaning.

1) "SourceFile is not specified": The caller did not specify an SourceFile parameter
2) "TargetFile is not specified": The caller did not specify an TargetFile parameter
3) "OutputFile is not specified": The caller did not specify an OutputFile parameter
3) "LogFolder is not specified": The caller did not specify a LogFolder parameter
4) "SourceFile not found: {SourceFile}": A SourceFile parameter was specified, but the file doesn't exist
5) "TargetFile not found: {TargetFile}": A TargetFile parameter was specified, but the file doesn't exist

All current DiffBuilderException have retryable set to false as they indicate some error with the input parameters.

Other exceptions thrown by the code are treated as fatal. Generally, in development if an exception occurs, it's because something is implemented
incorrectly.

Here are internal fatal exceptions we don't expect to see during execution in the DiffGeneration code:

1) Exception($"Unexpected HaslAlgorithmType: {algorithm}.") [ADUCreate.cs]
    Unknown type when converting between hashalgorith to hash type.
2) Exception($"Unexpected ArchiveItemType: {type}") [ADUCreate.cs]
    Unknown type when converting between archive item enum types (C# to native style)
3) Exception($"Unexpected RecipeType") [ADUCreate.cs]
    Unknown recipe type when converting from C# to native style
4) Exception($"File length of delta was negative. Source: {sourcePath}, Target: {targetPath}, Delta: {deltaPath}") [CreateDelta.cs]
    Should be impossible, but indicates something is very wrong when tyring to verify a created delta.
5) Exception($"Unexpected archive item type: {item.Type}") [DiffBuilder.cs]
    Utility function was passed an incorrect value.
6) Exception($"{this.GetType().Name}: didn't create file. Diff: {tempDelta}. BinaryPath: {process.StartInfo.FileName}, Args: {process.StartInfo.Arguments}") [ToolBaseDeltaBuilder.cs]
    Diff creation returned success, but there was no diff present - shouldn't happen. Proably fatal.
7) Exception($"Failed to move file {tempDelta} to {delta}") [ToolBaseDeltaBuilder.cs]    -- This may be retryable, depending on the reason
    We failed to move the delta after creating it in the temp location. 
8) JsonException with various parameters. Any JSON exception is a result of a bug in our code and can't be retried. There are many individual exceptions, but they 
   all indicating we are failing to write/read json that we have created ourselves.
9) NotImplementedException. A few locations in the code can throw this exception, but they shouldn't happen during normal execution. if they are seen then this indicates a bug and is not retryable.
10) IOException. It's possible for code to hit an IOException and generally these should be considered retryable.
11) System.Exception. There are many internal cases where we will throw a System.Exception besides the earlier listed cases. In general these are fatal and not retryable and indicate internal state has 
   gotten into a strange state.


Archive Loader
The Diff Generation code uses an ArchiveLoader to load archives before generating a diff. The ArchiveLoader is responsible for parsing archives
and creating a json that describes the content. To do this, the ArchiveLoader uses a set of archive libraries (CPIO, Tar, Ext4, SWU). The 
ArchiveLoader is initialized with a set of libraries and priority order, so that when it encounters an archive it iteratively tries each registered type.
Internally these archive libraries can throw exceptions, but these are generally hidden from the Diff Generation layer; an exception for a 
given archive typically indicates that a given archive library cannot process the input. The typical reason for this is that the library 
cannot load the archive, because it is not the matching type. The typical exceptions occuring here are FormatException type and don't indicate any
issues with Diff Generation, and will be caught by a catch block.

Here is an example code block generating an exception while processing a "New Ascii CpioArchive":
    [CpioArchives\NewAsciiCpioArchive.cs]
    var magicBytes = new ReadOnlySpan<byte>(rawHeaderData, 0, 6);
    string magic = Encoding.ASCII.GetString(magicBytes);

    if (magic.CompareTo(NewAsciiMagic) == 0)
    {
        subtype = "ascii.new";
    }
    else if (magic.CompareTo(NewcAsciiMagic) == 0)
    {
        subtype = "ascii.newc";
    }
    else
    {
        throw new FormatException($"Found \"{magic}\" instead of expected new ascii header \"{NewAsciiMagic}\" or newc header \"{NewcAsciiMagic}\"");
    }

This code ends up being caught and doesn't halt the execution. The exception is indicating that the given file doesn't begin with one of the two "magic" 
byte sequences which are used to identify a "New Ascii" file.

In the future, some of these exceptions may become visible an propagated up. The two types we are considering this for are IOException and exceptions
related to toolchain setup (for instance if the dependencies are missing); in the current implementation if DumpExtFs.exe is missing the ArchiverLoader
will never be able to load an Ext4 file, but the failure will look identical to as if the file wasn't an Ext4file - this is a gap that we'd like to fix.

