import os

srcDir='bouncer/src'
testDir='test/src'
clientDir='client/src'
wrapperDir='wrapper/src'
wrappertestDir='wrapper_test/src'


def printHeaderForCMakeFiles(file):

	file.write('#THIS IS AN AUTOMATICALLY GENERATED FILE - DO NOT OVERWRITE THE CONTENT!\n')
	file.write('#If you need to update the sources/headers/sub directory information, run updateSources.py at project root level\n')
	file.write('#If you need to import an extra library or something clever, do it on the CMakeLists.txt at the root level\n')
	file.write('#If you really need to overwrite this file, be aware that it will be overwritten if updateSources.py is executed.\n\n\n')
	return

def createCMakeList(dirName, files, subDirList, sourceName, headerName):
	cppInd = 0
	hInd = 0
	sources = {}
	headers = {}

	isOdaRequired = os.path.basename(dirName) == "odaHelper"

	for fname in sorted(files):
		if fname.lower().endswith('.cpp') or fname.lower().endswith('.cpp.inl'):
			sources[cppInd] = fname
			cppInd+=1
		if fname.lower().endswith('.h') or fname.lower().endswith('.hpp'):
			headers[hInd] = fname
			hInd+=1

	#populate a CMake file with this info

	cmakeFile = open(dirName+"/CMakeLists.txt", 'w')
	cmakeFile.truncate()

	printHeaderForCMakeFiles(cmakeFile)

	prefix = ""
	if isOdaRequired :
		cmakeFile.write("if(ODA_SUPPORT)\n")
		prefix = "\t"

	#include sub directories
	for subDir in sorted(subDirList):
		cmakeFile.write(prefix + 'add_subdirectory(' + subDir + ")\n")

	#write sources
	if len(sources.values()) > 0 :
		cmakeFile.write(prefix + 'set('+sourceName+'\n')
		cmakeFile.write(prefix + '\t${'+sourceName+'}\n')

		for fname in sorted(sources.values()):
			cmakeFile.write(prefix + '\t${CMAKE_CURRENT_SOURCE_DIR}/' + fname + '\n')

		cmakeFile.write(prefix + '\tCACHE STRING "'+sourceName+'" FORCE)\n\n')

	#write headers
	if len(headers.values()) > 0 :
		cmakeFile.write(prefix + 'set('+headerName+'\n')
		cmakeFile.write(prefix + '\t${'+headerName+'}\n')

		for fname in sorted(headers.values()):
			cmakeFile.write(prefix + '\t${CMAKE_CURRENT_SOURCE_DIR}/' + fname + '\n')

		cmakeFile.write(prefix + '\tCACHE STRING "'+headerName+'" FORCE)\n\n')

		if isOdaRequired :
			cmakeFile.write("endif()")

	cmakeFile.close()


	print('Created CMakeLists.txt in %s' %dirName)
	return



for dir, subDirList, fl in os.walk(srcDir):
	createCMakeList(dir, fl, subDirList, "SOURCES", "HEADERS")


for dir, subDirList, fl in os.walk(testDir):
	createCMakeList(dir, fl, subDirList, "TEST_SOURCES", "TEST_HEADERS")

for dir, subDirList, fl in os.walk(clientDir):
	createCMakeList(dir, fl, subDirList, "CLIENT_SOURCES", "CLIENT_HEADERS")
    
for dir, subDirList, fl in os.walk(wrapperDir):
	createCMakeList(dir, fl, subDirList, "WRAPPER_SOURCES", "WRAPPER_HEADERS")

for dir, subDirList, fl in os.walk(wrappertestDir):
	createCMakeList(dir, fl, subDirList, "WRAPPERTEST_SOURCES", "WRAPPERTEST_HEADERS")




