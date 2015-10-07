SET BOOST_ROOT=c:/local/boost_1_58_0/
SET BOOST_LIBRARYDIR=c:/local/boost_1_58_0/lib64-msvc-12.0/
SET MONGO_ROOT=c:/local/mongo-cxx-driver/
SET ASSIMP_ROOT=c:/local/assimp/

mkdir build
cd build
cmake -G "Visual Studio 12 Win64" ../
msbuild ALL_BUILD.vcxproj /v:quiet /p:Configuration=Release /t:Rebuild 
cd ..
