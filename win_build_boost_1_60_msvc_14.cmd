SET BOOST_ROOT=c:/local/boost_1_60_0/
SET BOOST_LIBRARYDIR=c:/local/boost_1_60_0/lib64-msvc-14.0/
SET MONGO_ROOT=c:/local/mongo-cxx-driver/
SET ASSIMP_ROOT=c:/local/assimp/

python updateSources.py

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=C:/local/3drepobouncer -G "Visual Studio 14 Win64" ../
msbuild INSTALL.vcxproj /v:quiet /p:Configuration=Release /t:Rebuild /clp:NoSummary;NoItemAndPropertyList;ErrorsOnly /nologo /p:WarningLevel=0 
msbuild INSTALL.vcxproj /v:quiet /p:Configuration=Debug /t:Rebuild /clp:NoSummary;NoItemAndPropertyList;ErrorsOnly /nologo /p:WarningLevel=0 
cd ..
