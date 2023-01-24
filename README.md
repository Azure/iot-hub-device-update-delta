This repo contains code for generating and applying Azure Device Update Diffs.
There are two main code-bases here:
<ol>
	<li>The Delta Processor
		<p>This code-base is written in C++ and deals with applying deltas. It also deals with low-level aspects of creating deltas.</p>
	</li>
	<li>the Diff Generation
		<p>This code-base is written in C# and deals with creating deltas. It deals with higher-level aspects such as recognizing
		formats of diffed items and selecting compression algorithms. It makes use of the Delta Processor and various tools for
		low-level tasks.</p>
	</li>
</ol>

<h1>Delta Processor</h1>
The Delta Processor code-base is written in C++ using C++17. The Delta Processor code deals with low level aspects of creating and apply deltas.

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


<h2>Building in Ubuntu 20.04</h2>
<h3>Install Dependencies</h3>
<h4>Apt packages</h4>
<p>These packages must be installed using apt. You may have to update apt to find all required packages.
<ul>
    <li>curl</li>
    <li>zip</li>
    <li>unzip</li>
    <li>tar</li>
    <li>g++</li>
    <li>gcc-9</li>
    <li>g++-9/li>
    <li>autoconf</li>
    <li>autopoint</li>
    <li>ninja-build</li>
    <li>pkg-config</li>
    <li>build-essential</li>
    <li>libtool</li>
    <li>cmake</li>
</ul>
<h4>Setting up alternatives for gcc/g++</h4>
<p>
You should setup alternatives for gcc and g++. Here are commands that will do this:
</p>
<ul>
<li>sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 20</li>
<li>sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 20</li>
</ul>
<h4>VCPKG dependencies</h4>
<p>We use several dependencies from VCPKG. Use the script at build/setup_vcpkg.sh to install the vcpkgs to a director of your choice. The script has two required and one optional arguments.
<ul>
    <li>&lt;vcpkg root&gt;: Required. Location to install the vcpkg dependencies</li>
    <li>&lt;port root&gt;: Required. Location for vcpkg ports. This is in the repo at vcpkg/ports</li>
    <li>[&lt;triplet&gt;]: Optional. The platform information for vcpkg installation - default is x64-linux.</li>
</ul>
</p>
<p>
Example: /mnt/c/code/ADU/build/setup_vcpkg.sh ~/vcpkg /mnt/c/code/ADU/vcpkg/ports/
When the script is finished running you should see output like this:
</p>
<p>
CMake arguments should be: -DCMAKE_TOOLCHAIN_FILE=/home/username/vcpkg/scripts/buildsystems/vcpkg.cmake
</p>
<h4>Using CMake<h4>
<p>
To build you'll use CMake. It's advised to create a separate folder inside of src. I typically use "out.linux". Once you've created the directory, cd into it and invoke cmake. After invoking cmake you should have a set of makefiles available and simply running "make" should build the binaries.
</p>

<h4>Running Tests</h4>
You can verify that things build correctly by running the tests. From your build output directory you can find the test at diffs/gtest/diffs_gtest.<br>

You will need to specify the path to the test data in the repo as well as the path to zstd_compress_file.<br>

Example: diffs/gtest/diffs_gtest --test_data_root /mnt/c/code/ADU/data --zstd_compress_file tools/zstd_compress_file/zstd_compress_file<br>

A successful run should see all tests passing, with an output like this:<br>

[----------] Global test environment tear-down<br>
[==========] 21 tests from 15 test suites ran. (10302 ms total)<br>
[  PASSED  ] 21 tests.<br>

<h2>Building in Windows</h2>
<p>Follow these steps to build:</p>
<ol>
    <li>Make sure Visual Studio is installed</li>
    <li>Use git to pull down the repo to a location, we'll use C:\code\adu-delta</li>
    <li>Select a location to install vcpkg, we'll use C:\code\vcpkg</li>
    <li>
        <p>Setup vcpkg using the script. Call C:\code\adu-delta\build\setup_vcpkg.ps1 with the repo locations for vcpkg and the ADU delta code.</p>
        <p>Example:  C:\code\adu-delta\build\setup_vcpkg.ps1 C:\code\vcpkg C:\code\adu-delta</p>
    </li>
    <li>Start up Visual Studio and open the folder C:\code\adu-delta\src as a CMake project. Select "C:\code\adu-delta\src\CMakeLists.txt" Via the menu: File->Open->CMake...</li>
    <li>Build the code via the menu: Build->Build</li>
</ol>

<h3>Pipeline Examples</h3>
<ul>
	<li>pipelines/CI/diffapi-ubuntu1804.yml</li>
	<li>pipelines/CI/diffapi-ubuntu2004-arm.yml</li>
	<li>pipelines/CI/diffapi-ubuntu2004-arm64.yml</li>
	<li>pipelines/CI/diffapi-ubuntu2004.yml</li>
	<li>pipelines/templates/build-native-linux.yml</li>
</ul>

<h1>Diff Generation</h1>

The Diff Generation code-base is written in C# in .NET 6.0, but leverages some native code.

<h2>Building in Ubuntu 20.04</h2>
<p>
To build the Diff Generation code in Linux, use the dotnet SDK:<br>
<ul>
    <li>sudo apt-get install -y dotnet-sdk-6.0</li>
    <li>dotnet buildbuild/diff-generation.sln</li>
</ul>
To clean the build output, use:
<ul>
    <li>dotnet clean build/diff-generation.sln</li>
</ul>
    And to run the unit tests on Linux:
<ul>
    <li>dotnet test src/managed/DiffGen/tests/UnitTests/UnitTests.csproj</li>
</ul>

<h2>Building in Windows</h2>
    <p>To build the Diff Generation C# code in Windows use a Visual Studio with support for .NET 6.0, such as Visual Studio 2022. Open up the solution at build/diff-generation.sln and build as normal.</p>

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
        Source Code: src/tools/zstd_compress_file
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
