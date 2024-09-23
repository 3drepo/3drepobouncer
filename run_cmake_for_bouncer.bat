

REM bouncer clone directory. Note that there are many dependency directors around as well.
SET PATH=C:\Applications\Python27;%PATH%
SET PATH=%PATH%;C:\Applications\Cmake-3.29.7\bin
SET BOOST_ROOT=C:\Dependencies\boost_1_59_0
SET BOOST_LIBRARYDIR=C:\Dependencies\boost_1_59_0\lib64-msvc-14.0
SET Boost_INCLUDE_DIR=C:\Dependencies\boost_1_59_0\boost
SET MONGO_ROOT=C:\Dependencies\mongo-cxx-driver
SET ASSIMP_ROOT=C:\Dependencies\assimp_5.0.1
SET OCCT_ROOT=C:\Dependencies\OCCT\opencascade-7.5.0\install
SET OCCT_LIB_DIR=C:\Dependencies\OCCT\opencascade-7.5.0\install\win64\vc14\libi
SET IFCOPENSHELL_ROOT=C:\Dependencies\IfcOpenShell-0.6.0
SET ODA_ROOT=C:\Dependencies\teigha_2025.8
SET ODA_LIB_DIR=C:\Dependencies\teigha_2025.8\lib\vc16_amd64dll
SET SYNCHRO_READER_ROOT=C:\Dependencies\3drepoSynchroReader
SET THRIFT_ROOT=C:\Dependencies\thrift
SET ZLIB_ROOT=C:\Dependencies\zlib

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat"
REM call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"


REM This line sets the current directory to the one this batch file is in
cd /d %~dp0
cd 3drepobouncer

python updateSources.py

REM If Tests are on, make sure to init the GoogleTest Git Submodule (git submodule init, git submodule update)
REM %CD% means the current directory, which should be the checkout

mkdir build
cd build
cmake -DREPO_BUILD_TESTS=ON -DODA_SUPPORT=ON -DSYNCHRO_SUPPORT=ON -DCMAKE_INSTALL_PREFIX="%CD%\install" -G "Visual Studio 16" -A x64 -T v142 ../
cd ..

cmd /k