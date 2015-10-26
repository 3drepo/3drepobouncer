SET BOOST_ROOT=c:/local/boost_1_58_0/
SET BOOST_LIBRARYDIR=c:/local/boost_1_58_0/lib64-msvc-12.0/
SET MONGO_ROOT=c:/local/mongo-cxx-driver/
SET ASSIMP_ROOT=c:/local/assimp/

python updateSources.py

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=C:/local/3drepobouncer -G "Visual Studio 12 Win64" ../
msbuild INSTALL.vcxproj /v:quiet /p:Configuration=Release /t:Rebuild
msbuild INSTALL.vcxproj /v:quiet /p:Configuration=Debug
cd ..
