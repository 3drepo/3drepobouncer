#!/usr/bin/python

import sys
import os

def fatalError(message):
    print message
    sys.exit()

numArguments = len(sys.argv)

if numArguments < 5:
    fatalError("Usage: " + sys.argv[0] + " <prod/dev> <major> <minor1> <minor2>")

release_type = sys.argv[1]
majorV      = sys.argv[2]
minor1V      = sys.argv[3]
minor2V      = sys.argv[4]
version = majorV + "." + minor1V + "." + minor2V

production   = (release_type == "prod")
branch       = "master" if production else "staging"

os.system("cd ../..")

code = os.system("git checkout " + branch)

if code:
    fatalError("git checkout failed")

if code:
    fatalError("git force add failed")


os.system("sed 's/VERSION_MAJOR [^ ]*/VERSION_MAJOR " + majorV + ")/' bouncer/CMakeLists.txt > bouncer/CMakeLists.txt.bak")
os.system("mv bouncer/CMakeLists.txt.bak bouncer/CMakeLists.txt")

minorTag = minor1V +"_"+minor2V

os.system("sed 's/VERSION_MINOR [^ ]*/VERSION_MINOR " + minorTag + ")/' bouncer/CMakeLists.txt > bouncer/CMakeLists.txt.bak")
os.system("mv bouncer/CMakeLists.txt.bak bouncer/CMakeLists.txt")

os.system("sed 's/BOUNCER_VMAJOR [^ ]*/BOUNCER_VMAJOR " + majorV + "/' bouncer/src/repo/repo_bouncer_global.h > bouncer/src/repo/repo_bouncer_global.h.bak")
os.system("mv bouncer/src/repo/repo_bouncer_global.h.bak bouncer/src/repo/repo_bouncer_global.h")
minorVersion = minor1V + "." + minor2V
os.system("sed 's/BOUNCER_VMINOR [^ ]*/BOUNCER_VMINOR \"" + minorVersion + "\"/' bouncer/src/repo/repo_bouncer_global.h > bouncer/src/repo/repo_bouncer_global.h.bak")
os.system("mv bouncer/src/repo/repo_bouncer_global.h.bak bouncer/src/repo/repo_bouncer_global.h")


os.system("git add bouncer/CMakeLists.txt bouncer/src/repo/repo_bouncer_global.h")

os.system("git commit -m \"Version " + version + "\"")

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

