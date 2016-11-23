#!/usr/bin/python

import sys
import os

def fatalError(message):
    print message
    sys.exit()

numArguments = len(sys.argv)

if numArguments < 4:
    fatalError("Usage: " + sys.argv[0] + " <prod/dev> <major> <minor1> <minor2>")

release_type = sys.argv[1]
majorV      = sys.argv[2]
minor1V      = sys.argv[3]
minor2V      = sys.argv[4]
version = majorV + "." + minor1V + "." + minor2V

production   = (release_type == "prod")
branch       = "release" if production else "master"

os.system("cd ../..")

code = os.system("git checkout " + branch)

if code:
    fatalError("git checkout failed")

if code:
    fatalError("git force add failed")

os.system("git commit -m \"Version " + version + "\"")

os.system("sed -i 's/VERSION_MAJOR [^ ]*/VERSION_MAJOR " + majorV + ")/' bouncer/CMakeLists.txt")

minorTag = minor1V +"_"+minor2V

os.system("sed -i 's/VERSION_MAJOR [^ ]*/VERSION_MINOR " + minorTag + ")/' bouncer/CMakeLists.txt")

os.system("sed -i 's/BOUNCER_VMAJOR [^ ]*/BOUNCER_VMAJOR " + majorV + "/' bouncer/src/repo/repo_bouncer_global.h")
minorVersion = minor1V + "." + minor2V
os.system("sed -i 's/BOUNCER_VMINOR [^ ]*/BOUNCER_VMINOR \"" + minorVersion + "\"/' bouncer/src/repo/repo_bouncer_global.h")


os.system("git add bouncer/CMakeLists.txt bouncer/src/repo/repobouncer_global.h")
os.system("git clean -f -d")

os.system("git commit -m \"Version string update\"")

os.system("git push origin :refs/tags/" + version)
os.system("git tag -fa " + version + " -m \" Version " + version + " \"")

if production:
    os.system("git push origin :refs/tags/latest")
    os.system("git tag -fa latest -m \"Update latest\"")
else:
    os.system("git push origin :refs/tags/dev_latest")
    os.system("git tag -fa dev_latest -m \"Update latest\"")

os.system("git push origin --tags")
os.system("git push")

