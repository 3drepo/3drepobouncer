SET BOOST_ROOT=c:/local/boost_1_60_0/
SET BOOST_LIBRARYDIR=c:/local/boost_1_60_0/lib64-msvc-12.0/
SET MONGO_ROOT=c:/local/mongo-cxx-driver/
SET ASSIMP_ROOT=c:/local/assimp/
SET OCCT_ROOT=c:/local/occt 
SET OCCT_LIB_DIR=c:/local/occt/win64/vc12/lib
SET OCCT_DEBUG_LIB_DIR=c:/local/occt/win64/vc12/libd
SET IFCOPENSHELL_ROOT=c:/local/IfcOpenShell

python updateSources.py

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=C:/local/3drepobouncer -G "Visual Studio 12 Win64" ../
msbuild INSTALL.vcxproj /v:quiet /p:Configuration=Release /t:Rebuild /clp:NoSummary;NoItemAndPropertyList;ErrorsOnly /nologo /p:WarningLevel=0 
msbuild INSTALL.vcxproj /v:quiet /p:Configuration=Debug /t:Rebuild /clp:NoSummary;NoItemAndPropertyList;ErrorsOnly /nologo /p:WarningLevel=0 
cd ..
