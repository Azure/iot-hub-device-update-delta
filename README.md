This repo contains code for generating and applying Azure Device Update Diffs.
There are two main code-bases here. The code for Diff Generation and is located 
at src/managed/DiffGen, while the code for Diff Application is at src/diffs.

<h1>Diff Generation</h1>

The Diff Generation code-base is written in C# in .NET 6.0, but leverages some native code.

<h2>Components</h2>

The following represent the main components for Diff Generation:
<ol>
    <li>ArchiveUtility
        <p>Source Code: src/managed/DiffGen/ArchiveUtility</p>
        <p>This module is written in C# using .NET 6.0; it contains basic building blocks used for parsing and 
        describing archive files. The module includes a basic object model to describe Archives in terms of 
        Chunks, Payloads and Recipes and an ArchiveLoader which is an extensible mechanism to load Archive files.</p>
    </li>
    <li>DiffGeneration
        <p>Source Code: src/managed/DiffGen/DiffGeneration</p>
        <p>This module is written in C# using .NET 6.0; it contains various methods to compare and transform items
        in archives. The goal of this module is to create a diff between a source and target archive by leveraging
        the information from their object models and utilize 3rd party compression (zstd, zlib, bsdiff) in a targeted
        way. When the work has been done, the code will then using PINVOKEs to callin to the C-API exports provided
        by the Diff Application code to serialize the object model into a binary form.</p>
    </li>    
    <li>archive implementations
        <p>Source Code: src/managed/DiffGen/archives</p>
        <p>There are several archives implementing a few different archive types which are used for updates. Each of them 
        are separate implementations of IArchive from the ArchiveUtility module. Each implementation allows for adding
        new archive types to ArchiveLoader, so we can extend the behavior.
        Example supported archive types: cpio, tar, SWUpdate, ext4
        These modules are written in C# using .NET 6.0.</p>
    </li>
    <li>demos
        <p>Source Code: src/managed/DiffGen/demos</p>
        <p>There are a few demos used for Diff Generation. These are used to test/demonstrate features.
        These demos are written in C# using .NET 6.0.
        </p>
    </li>
    <li>tests
        <p>Source Code: src/managed/DiffGen/tests</p>
        <p>There are a few tests used to test unit test behaviors related to diff generation.
        These tests are written in C# using .NET 6.0.</p>
    </li>
    <li>DiffGenTool
        <p>Source Code: src/managed/DiffGen/tools/DiffGenTool</p>
        <p>
        </p>
    </li>
The Diff Generation solution depends on several pieces of native code.
<ol>
    <li> DumpExtFs
        <p>Source Code: src/tools/dumpextfs</p>
        <p>This tool is written in C++ using C++17 and is build in Linux using some scripts in the folder.
        It can be build for Linux or Cross-compiled to win32 using Mingw.        
        This tool links with assets from the e2fsprogs project with some slight modifications - a git .patch files
        is included.
        The tool is used to process ext4 files by the managed ext4 archive module. This tool has a simple usage - 
        simply supply a ext4 archive to the tool and it will output an JSON to stdout, or write to a file by supplying
        an additional parameter.
        </p>
        <p>
        This module uses two libraries from e2fsprogs.
        </p>
    </li>
    <li> zstd_compress_file
        Source Code: src
        <p>
        </p>
    </li>
    <li> Diff API
        <p>Source Code: src/diffs/api</p>
        <p>This is the C API module that implements diff creation and application.
        It wraps the rest of the native code which is written in C++. We provide a C API wrapper to enable easy PInvokes for the C# code.
        </p>
    </li>
</ol>

<h1>Diff API</h1>
The Diff API code-base is written in C++ using C++17. 

<h2>Open Source Dependencies</h2>
The code uses a few open source libraries:
<ol>
<li>BSDiff</li>
<li>Bzip2 (via BSDiff)</li>
<li>libgcrypt (linux only)</li>
<li>ligpg-error (linux only)</li>
<li>ms-gsl</li>
<li>ZLIB</li>
<li>ZSTD</li>
</ol>

<h2>Components</h2>
The following represent the major components of Diff Application:
<ol>
    <li>diff lib
        <p>Source Code: src/diffs</p>
        <p>This module is written in C++ using C++17.
        This is the main module for implementing diffs. This code has objects to represent the diff,
        archive_items (chunks, blobs, payload), and recipes; these exists in parallel to the objects that 
        are present in the DiffGeneration code-base, as they represent the same ideas. The a diff is composed of
        a set of one or more chunks, which themselves contain recipes, which may refer to more archive_items; the diff
        ends up with a tree-like structure.
        While applying the diff we must traverse the tree of archive_items and recipes and write them to the 
        desired location. To accomplish this we create an apply_context object that can be mutated as it visits the
        tree; the mutations will allow us to read/write data for the diff application appropriately without having to 
        modify any state on the diff items.
        </p>
    </li>
    <li>diff api
        <p>Source Code: src/diffs/api</p>
        <p>The code compiles with default C options for gcc and Visaul Studio, but should be compatible with C11 and C17.
        A C API to expose apply and create functionality from the C++ code in the diff lib. This allows both for
        PINVOKE from C# code and also calling from applications written in C.
        The headers for the code here expose a simple session model for the API with create/close APIs and
        other APIs for interacting with the handle to apply or create diffs.</P>
    </li>    
    <li>hash_utility
        <p>Source Code: src/hash_utility</p>
        <p>This module is written in C++ using C++17.
        A module to support hashing bytes in a cross-platform manner. It has #ifdefs for WIN32 to use bcrypt and otherwise
        uses mhash for Linux. We want to isolate any code that has to use #ifdefs from the rest of the code.</p>
    </li>
    <li>io_utility
        <p>Source Code: src/io_utility</p>
        <p>This module is written in C++ using C++17.
        A module to support reading and writing from streams. The streams/objects here are not compatible with boost or C++ streams, but instead are specific to this implementation. Streaming to/from files is supported as well as streaming with various compression types (zstd, zlib, bsdiff).
        </p>
    </li>
    4) applydiff
        <p>Source Code: src/tools/applydiff</p>
        <p>This tool is written in C. The code compiles with default C options for gcc and Visaul Studio, but should be compatible with C11 and C17.
        This tool is a simple wrapper around the diff api. It is used as a proof of concept for the API and
        also a testing tool in development.</p>
</ol>

Building in Linux:
    To build the Diff Generation code in Linux, use the dotnet SDK:
        sudo apt-get install -y dotnet-sdk-6.0
        dotnet buildbuild/diff-generation.sln
    To clean the build output, use:
        dotnet clean build/diff-generation.sln
    And to run the unit tests on Linux:
        dotnet test src/managed/DiffGen/tests/UnitTests/UnitTests.csproj
    To build the native C++ tools, preferably use CMAKE at the AzureDeviceUpdateDiffs/src/out folder.
