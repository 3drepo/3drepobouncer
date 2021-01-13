#!/usr/bin/python

import sys
import os
import re, shutil, tempfile

def execExitOnFail(cmd, message):
    code = os.system(cmd);
    if code:
        fatalError(message);

def fatalError(message):
    print message
    sys.exit()

def sedCmd(pattern, repl, filename):
    '''
    Perform the pure-Python equivalent of in-place `sed` substitution: e.g.,
    `sed -i -e 's/'${pattern}'/'${repl}' "${filename}"`.
    '''
    # For efficiency, precompile the passed regular expression.
    pattern_compiled = re.compile(pattern)

    # For portability, NamedTemporaryFile() defaults to mode "w+b" (i.e., binary
    # writing with updating). This is usually a good thing. In this case,
    # however, binary writing imposes non-trivial encoding constraints trivially
    # resolved by switching to text writing. Let's do that.
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as tmp_file:
        with open(filename) as src_file:
            for line in src_file:
                tmp_file.write(pattern_compiled.sub(repl, line))

    # Overwrite the original file with the munged temporary file in a
    # manner preserving file attributes (e.g., permissions).
    shutil.copystat(filename, tmp_file.name)
    shutil.move(tmp_file.name, filename)


def updateCmake(majorV, minorTag):
    sedCmd("VERSION_MAJOR [^ ]*", "VERSION_MAJOR " + majorV +")\n", "bouncer/CMakeLists.txt");
    sedCmd("VERSION_MINOR [^ ]*", "VERSION_MINOR " + minorTag + ")\n", "bouncer/CMakeLists.txt");
    execExitOnFail("git add bouncer/CMakeLists.txt", "failed to add cmake to git")

def updateSrcHeaders(majorV, minorTag):
    sedCmd("BOUNCER_VMAJOR [^ ]*", "BOUNCER_VMAJOR " + majorV + "\n", "bouncer/src/repo/repo_bouncer_global.h");
    sedCmd("BOUNCER_VMINOR [^ ]*", "BOUNCER_VMINOR \"" + minorTag +"\"\n", "bouncer/src/repo/repo_bouncer_global.h");
    execExitOnFail("git add bouncer/src/repo/repo_bouncer_global.h", "failed to add repo_bouncer_global.h to git");

def updateBouncerWorker(version):
    sedCmd("\"version\": [^ ]*", "\"version\": \"" + version + "\",\n", "tools/bouncer_worker/package.json");
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

