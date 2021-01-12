#!/usr/bin/python

import sys
import os
import re

def execExitOnFail(cmd, message):
    code = os.system(cmd);
    if code:
        fatalError(message);

def fatalError(message):
    print message
    sys.exit()

def sedCmd(sourceExp, desExp, file):
    if os.name == "nt":
        return "powershell.exe \"(Get-content "+ file +") | ForEach-Object {$_ -creplace \'" + sourceExp +"\', \'" +desExp +"\'} | Set-Content " + file + "\""
    else:
        return "sed -i \'.bak\' \'s/" + sourceExp + "/" + desExp + "/g\' " + file

def updateCmake(majorV, minorTag):
    updateMajor = sedCmd("VERSION_MAJOR [^ ]*", "VERSION_MAJOR " + majorV +")", "bouncer/CMakeLists.txt");
    execExitOnFail(updateMajor, "Failed to update major number on cmake");

    updateMinor =  sedCmd("VERSION_MINOR [^ ]*", "VERSION_MINOR " + minorTag + ")", "bouncer/CMakeLists.txt");
    execExitOnFail(updateMinor, "Failed to update minor number on cmake");
    execExitOnFail("git add bouncer/CMakeLists.txt", "failed to add cmake to git")

def updateSrcHeaders(majorV, minorTag):
    updateMajor = sedCmd("BOUNCER_VMAJOR [^ ]*", "BOUNCER_VMAJOR " + majorV, "bouncer/src/repo/repo_bouncer_global.h");
    execExitOnFail(updateMajor, "Failed to update major number in source");

    updateMinor =  sedCmd("BOUNCER_VMINOR [^ ]*", "BOUNCER_VMINOR \"" + minorTag +"\"", "bouncer/src/repo/repo_bouncer_global.h");
    print updateMinor
    execExitOnFail(updateMinor, "Failed to update minor number in source");
    execExitOnFail("git add bouncer/src/repo/repo_bouncer_global.h", "failed to add repo_bouncer_global.h to git");

def updateBouncerWorker(version):
    updateVersion = sed("\"version\":[^ ]*", "\"version\": \"" + version + "\",", "tools/bouncer_worker/package.json");
    execExitOnFail(updateVersion, "Failed to update version in package.json");
    execExitOnFail("git add tools/bouncer_worker/package.json", "failed to add package.json to git");

numArguments = len(sys.argv)

if numArguments < 3:
    fatalError("Usage: " + sys.argv[0] + " <prod/dev> <versionNumber>")

release_type = sys.argv[1]
version = sys.argv[2]

versionArray = version.split(".");
majorV      = versionArray[0]
minor1V      = versionArray[1]
minor2V      = versionArray[2]

production   = (release_type == "prod")
branch       = "master" if production else "staging"

minorTag = minor1V + "_" + minor2V

updateCmake(majorV, minorTag);
updateSrcHeaders(majorV, minorTag);
updateBouncerWorker(version)

sys.exit(0);

execExitOnFail("git clean -f -d", "Failed to clean directory")


execExitOnFail("git commit -m \"Version " + version + "\"", "Failed to commit")

execExitOnFail("git push origin :refs/tags/" + version, "Failed to push tag")
execExitOnFail("git tag -fa " + version + " -m \" Version " + version + " \"", "Failed to add tag")

if production:
    execExitOnFail("git push origin :refs/tags/latest","Failed to push tag")
    execExitOnFail("git tag -fa latest -m \"Update latest\"", "Failed to add tag")
else:
    execExitOnFail("git push origin :refs/tags/dev_latest", "Failed to push tag")
    execExitOnFail("git tag -fa dev_latest -m \"Update latest\"", "Failed to add tag")

execExitOnFail("git push origin --tags", "Failed to update tag")
execExitOnFail("git push", "Failed to push upstream")

