import os

srcDir='src'



def createCMakeList(dirName, files, subDirList):
	cppInd = 0
	hInd = 0
	sources = {}
	headers = {}
	for fname in files:
		if fname.lower().endswith('.cpp'):
			sources[cppInd] = fname
			cppInd+=1
		if fname.lower().endswith('.h'):
			headers[hInd] = fname
			hInd+=1
		
	#populate a  CMake file with this info
	
	cmakeFile = open(dirName+"/CMakeLists.txt", 'w')
	cmakeFile.truncate()

	#include sub directories
	for subDir in subDirList:
		cmakeFile.write('add_subdirectory(' + subDir + ")\n")

	#write sources
	if len(sources.values()) > 0 :
		cmakeFile.write('set(SOURCES\n')
		cmakeFile.write('\t${SOURCES}\n')

		for fname in sources.values():
			cmakeFile.write('\t${CMAKE_CURRENT_SOURCE_DIR}/' + fname + '\n')
		
		cmakeFile.write('\tCACHE STRING "source files" FORCE)\n\n')

	#write headers
	if len(headers.values()) > 0 :
		cmakeFile.write('set(HEADERS\n')
		cmakeFile.write('\t${HEADERS}\n')
	
		for fname in headers.values():
			cmakeFile.write('\t${CMAKE_CURRENT_SOURCE_DIR}/' + fname + '\n')
		
		cmakeFile.write('\tCACHE STRING "header files" FORCE)\n\n')

	cmakeFile.close()


	print('Created CMakeLists.txt in %s' %dirName)
	return



for dir, subDirList, fl in os.walk(srcDir):
	createCMakeList(dir, fl, subDirList)
