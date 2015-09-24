3drepobouncer
=========

3DRepoBouncer (temporary naming) is essentially the refactored 3DRepoCore and (parts of) 3DRepoGUI. A C++ library providing 3D Repo Scene Graph definition, repository management logic and manipulation logic. 

Dependencies
------------
3DRepoBouncer relies on the following libraries:
* [Boost Library v1.58.0+](http://www.boost.org/)  
See also [pre-built binaries for Windows](http://sourceforge.net/projects/boost/files/boost-binaries/)
and [modular Boost](https://svn.boost.org/trac/boost/wiki/ModularBoost)
on [GitHub](https://github.com/boostorg)
* [Mongo CXX Driver Legacy v1.0+](https://github.com/mongodb/mongo-cxx-driver)
See [3D Repo guide for compiling MongoDB](https://github.com/3drepo/3drepobouncer/wiki/Compiling-MongoDB)
* [3D Repo ASSIMP library fork](https://github.com/3drepo/assimp)

To compile and install the library, the following are used:
* [Python v2.x](https://www.python.org/)
* [CMAKE v3.3] (http://www.cmake.org/)
* A C++ compiler (GNU GCC v3.4+, [Visual Studio 12+](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx))

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
Apart from the `CMakeLists.txt` at root level, eveyr other cmake file is automatically generated. If you have moved/created any source/header files, please run `python updateSources.py` to update the `CMakeLists.txt` files within the subdirectories before compiling.

Do NOT modify any `CMakeLists.txt` files within src folder as any changes will be overwritten when `updateSources.py` is executed!

Contact
-------

If you need any help or want to contribute please contact: `support@3drepo.org`
We look forward to hearing from you.

[3DRepoIO]: https://github.com/3drepo/3drepo.io
[3DRepoGUI]: https://github.com/3drepo/3drepogui
