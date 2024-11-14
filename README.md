3drepobouncer [![Build Status](https://travis-ci.org/3drepo/3drepobouncer.svg?branch=master)](https://travis-ci.org/3drepo/3drepobouncer) [![Coverage Status](https://coveralls.io/repos/github/3drepo/3drepobouncer/badge.svg?branch=master)](https://coveralls.io/github/3drepo/3drepobouncer?branch=master)
=========

3DRepoBouncer is essentially the refactored 3DRepoCore and (parts of) 3DRepoGUI. It is a C++ library providing 3D Repo Scene Graph definition, repository management and manipulation logic as well as direct MongoDB database access.

### Latest Releases
We always recommend using the [Latest stable release](https://github.com/3drepo/3drepobouncer/releases). However, to access cutting-edge development versions, check out the [tags](https://github.com/3drepo/3drepobouncer/tags).

## Licenses
This project is Copyright of [3D Repo Ltd](http://3drepo.org), a company registered in England and Wales No. 09014101, and is released under the open source [GNU Affero General Public License v3](http://www.gnu.org/licenses/agpl-3.0.en.html). Should you require a commercial license, please contact [support@3drepo.org](mailto:support@3drepo.org). All contributors are required to sign either the [3D Repo Individual](https://gist.github.com/jozefdobos/e177af804c9bcd217b73) or the [3D Repo Entity](https://gist.github.com/jozefdobos/c7c4c1c18cfb211c45d2) [Contributor License Agreement (CLA)](https://en.wikipedia.org/wiki/Contributor_License_Agreement).

### Contributing
We very much encourage contributions to the 3D Repo project. Firstly, fork the desired repository and commit your modifications there. Once happy with the changes, you can generate a [pull request](https://help.github.com/articles/using-pull-requests/) and our team will integrate it upstream after a review.

Your pull requests should:

1. Follow the style of the existing code
2. One commit should just do one thing, and one thing only
3. Work in a branch assigned to a specific issue number, e.g. branch called "ISSUE_138"
4. Each commit message should be prefixed with the issue number, e.g. "#138 Fixing bug xyz..."
5. Rebase your branch against [upstream's master](https://help.github.com/articles/merging-an-upstream-repository-into-your-fork/) so that we don't pull redundant commits
6. Sign our [3D Repo Individual CLA](https://gist.github.com/jozefdobos/e177af804c9bcd217b73) or if you are representing a legal entity, sign the [3D Repo Entity CLA](https://gist.github.com/jozefdobos/c7c4c1c18cfb211c45d2)

## API Documentation
Doxygen documentation can be found [here](http://3drepo.github.io/3drepobouncer/doc/html/)

Dependencies
------------
3DRepoBouncer relies on the following libraries:
* [Boost Library v1.55.0 - v1.60.0](http://www.boost.org/)
See also [pre-built binaries for Windows](http://sourceforge.net/projects/boost/files/boost-binaries/)
and [modular Boost](https://svn.boost.org/trac/boost/wiki/ModularBoost)
on [GitHub](https://github.com/boostorg)
* [Mongo CXX Driver Legacy v1.0+](https://github.com/mongodb/mongo-cxx-driver)
See [3D Repo guide for compiling MongoDB](https://github.com/3drepo/3drepobouncer/wiki/Compiling-MongoDB-CXX-Driver)
* [ASSIMP](https://github.com/assimp/assimp)
See [3D Repo guide for compiling ASSIMP](https://github.com/3drepo/3drepobouncer/wiki/Compiling-ASSIMP)
* [IFCOpenShell](https://github.com/IfcOpenShell/IfcOpenShell)

#### Optional Dependencies
The following are optional dependencies depending on your configuration
* 3D Repo Synchro Reader - 3D Repo's proprietary library for reading Synchro SPM files.
* [Teigha Drawing and BIM library](https://www.opendesign.com/) - Proprietary library for reading RVT, NWD/NWC, DWG and DGN files

To compile and install the library, the following are used:
* [Python v2.x](https://www.python.org/)
* [CMAKE v3.x] (http://www.cmake.org/)
* A C++ compiler (GNU GCC v4.9+, [Visual Studio 12+](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx))

Compilation (Qt)
------------
See instructions at [https://github.com/3drepo/3drepobouncer/issues/9#issuecomment-135727927](https://github.com/3drepo/3drepobouncer/issues/9#issuecomment-135727927)

Compilation (Windows)
------------
Ensure Mongo CXX Driver, Boost and ASSIMP libraries are installed.

The following instruction is for compiling a 64bit library using Visual Studio 12's tools. Change the pathing/cmake option appropriately if you are using another version of Visual Studio or compiling with a different compiler.

In command line prompt:
set the following environmental variables to the directories of your installations:
* `$env:BOOST_ROOT = path_to_boost`
* `$env:BOOST_LIBRARYDIR = path_to_boost_libraries (i.e path_to_boost\lib64-msvc-12.0)` (NOT REQUIRED if libraries are in `$BOOST_ROOT/lib`)
* `$env:MONGO_ROOT = path_to_mongo_cxx_driver`
* `$env:ASSIMP_ROOT = path_to_assimp`
* `$env:OCCT_ROOT = path_to_occt`
* `$env:OCCT_LIB_DIR = path_to_occt_libraries (e.g path_to_occt\win64\vc14\lib)` (NOT REQUIRED if libraries are in `$OCCT_ROOT/lib`)
* `$env:IFCOPENSHELL_ROOT = path_to_ifcOpenShell`

You will also require the following if you are building with `ODA_SUPPORT` checked:
* `$env:ODA_ROOT`
* `$env:ODA_LIB_DIR` (e.g. `$ODA_ROOT/lib/lnxX64_5.3dll/`)

You will also require the following if you are building with `SYNCHRO_SUPPORT` checked:
* `$env:SYNCHRO_ROOT`
* `$env:THRIFT_ROOT`
* `$env:SYNCHRO_READER_ROOT`

1. Clone the repository: `git clone https://github.com/3drepo/3drepobouncer.git`
2. Change directory: `cd 3drepobouncer`
3. Update CMake files: `python updateSources.py`
4. Create off-source build directory: `mkdir build`
5. Change directory: `cd build`
6. Configure build with CMAKE: `cmake -G "Visual Studio 12 Win64" ../`
7. Build the library: `msbuild 3drepobouncer.vcxproj`

Compilation (Linux)
------------
Ensure Mongo CXX Driver, Boost and ASSIMP libraries are installed.

If the libraries are not installed in `/usr, /usr/local, /opt/local,` set the following environmental variables:
* `export BOOST_ROOT = path_to_boost`
* `export MONGO_ROOT = path_to_mongo_cxx_driver`
* `export ASSIMP_ROOT = path_to_assimp`

1. Clone the repository: `git clone https://github.com/3drepo/3drepobouncer.git`
2. Change directory: `cd 3drepobouncer`
3. Update CMake files: `python updateSources.py`
4. Create off-source build directory: `mkdir build`
5. Change directory: `cd build`
6. Configure build with CMAKE: `cmake ../`
7. Build the library: `make`

Recompiling with changes
------------
Apart from the `CMakeLists.txt` at root level, every other cmake file is automatically generated. If you have moved/created any source/header files, please run `python updateSources.py` to update the `CMakeLists.txt` files within the subdirectories before compiling.

Do NOT modify any `CMakeLists.txt` files within src folder as any changes will be overwritten when `updateSources.py` is executed!

Contact
-------

If you need any help or want to contribute please contact: `support@3drepo.org`
We look forward to hearing from you.

[3DRepoIO]: https://github.com/3drepo/3drepo.io
[3DRepoGUI]: https://github.com/3drepo/3drepogui
