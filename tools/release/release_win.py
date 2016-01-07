
#============== CONSTANTS DECLARATIONS - CHANGE AS NESCCESSARY =================
#NOTE: This script assumes the following:
#  1. Envar MONGO_ROOT is set
#  2. Envar ASSIMP_ROOT is set
#  3. Envar BOOST_LIBRARYDIR is set
#  4. the command "git" is in $PATH
#  5. the command "cmake" is in $PATH
#  6. the command "msbuild" is in $PATH

#github repository
githubRepo = "https://github.com/3drepo/3drepobouncer"

#Release tag (Also used to name the folder)
releaseTag = "v1.0a"

#name
projName = "3drepobouncer"
bouncerdll = "3drepobouncer_1_0a.dll"

#Visual Studio Version (Tag as you expect to type to -G on Cmake)
vsVersion = "\"Visual Studio 12 Win64\""

#Name of mongo dll
mongodll = "mongoclient.dll"

#Name of assimp dll
assimpdll = "assimp-vc120-mt.dll"

#Names of boost dlls
boostdlls = ["boost_chrono-vc120-mt-1_58.dll", "boost_date_time-vc120-mt-1_58.dll", "boost_filesystem-vc120-mt-1_58.dll", "boost_log_setup-vc120-mt-1_58.dll","boost_log-vc120-mt-1_58.dll","boost_program_options-vc120-mt-1_58.dll","boost_regex-vc120-mt-1_58.dll","boost_system-vc120-mt-1_58.dll","boost_thread-vc120-mt-1_58.dll"]
#===============================================================================

import os

rootDir = os.getcwd() + "\\" + releaseTag
workSpaceDir = rootDir + "\\workspace"
installDir = rootDir

#Create output directory
os.system("mkdir \"" + rootDir + "\"")
os.system("mkdir \"" + workSpaceDir + "\"")

os.chdir(workSpaceDir)

#Get fresh code base from git & compile and Install
os.system("git clone --branch=" + releaseTag + " " + githubRepo) #Change it to only get the tagged version
os.system("mkdir build")
os.chdir("build")

os.system("cmake -DCMAKE_INSTALL_PREFIX=\"" + installDir + "\" -G " + vsVersion + " ..\\" + projName)
os.system("msbuild INSTALL.vcxproj /p:configuration=Release")

os.chdir(rootDir)

#Remove workspace
os.system("rmdir \""+ workSpaceDir + "\" /S /Q")

#Copy libraries
fpMongodll = os.environ["MONGO_ROOT"] + "\\lib\\" + mongodll
fpAssimpdll = os.environ["ASSIMP_ROOT"] + "\\lib\\" + assimpdll

os.system("copy \"" + fpMongodll + "\" \"" + installDir + "\\bin\"")
os.system("copy \"" + fpAssimpdll + "\" \"" + installDir + "\\bin\"")

boostLib = os.environ["BOOST_LIBRARYDIR"]
for boostDll in boostdlls:
	os.system("copy \"" + boostLib + "\\" + boostDll + "\" \"" + installDir + "\\bin\"")

os.system("copy \"" + installDir + "\\lib\\" + bouncerdll + "\" \"" + installDir + "\\bin\"")

#copy README and licensing info
os.system("copy \"" + rootDir + "\\..\\README.txt\" .")
os.system("mkdir license")
os.system("copy \"" + rootDir + "\\..\\license\" license")

