import glob, os
import sys, re, hashlib
from datetime import datetime
from subprocess import Popen, PIPE
from threading import Timer


processTimeout = 360; #1 hr runtime

def findAllFiles(path, ext):
    filesToTest = [];
    for root, dirs, files in os.walk(path):
        for file in files:
            if file.endswith("." + ext):
                filesToTest.append(os.path.join(root, file));
    return filesToTest;

def determineResult(file, logDir):
    code = -1
    logFiles = glob.glob(os.path.join(logDir, "*"));
    if len(logFiles) > 0:
        logFile = logFiles[0];
        with open(logFile, 'r') as f:
            lines = f.read().splitlines()
            lastLine = lines[-1]
            regex = re.search(".+returning with error code: ", lastLine);
            if regex != None:
                codeStr = lastLine[regex.end():]
                code = int(codeStr);
    else:
        code = "NOT RAN"
    return [file, code, logDir]

def writeResults(file, results):
    fstream = open(file, "w");
    fstream.write("File,Exit Code,Log location\n");
    for entry in results:
        fstream.write(entry[0] + "," + str(entry[1]) + ","+ entry[2] + "\n");
    fstream.close();

if len(sys.argv) < 5:
    print "Usage: " + sys.argv[0] + " <extension> <file directory> <log directory> <output file>"
    sys.exit(-1);

ext = sys.argv[1];
path = sys.argv[2];
logFolder = sys.argv[3];
outFile = sys.argv[4];

files = findAllFiles(path, ext);
files.sort()
results = [];

count = 0;
total = len(files);

for file in files:
    count+=1;
    logPath = os.path.join(logFolder, str(count));
    results.append(determineResult(file, logPath));

writeResults(outFile, results);




