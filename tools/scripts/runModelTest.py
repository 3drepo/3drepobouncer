import glob, os
import sys, re, hashlib, timeit
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

def runImportCmd(file, logDir):
    os.environ["REPO_LOG_DIR"] = logDir;
    proc = Popen(["3drepobouncerTool.exe", "testImport" ,file], stdout=PIPE, stderr=PIPE);
    timer = Timer(processTimeout, proc.kill)
    timeStart = timeit.default_timer();
    try:
        timer.start();
        proc.wait();
    finally:
        timer.cancel();
    timeEnd = timeit.default_timer();
    timeTaken = timeEnd - timeStart;

    if timeTaken > processTimeout:
        code = "TIMED OUT"
    else:
        code = proc.returncode

    # for some reason (I suspect ODA), windows is no longer returning an accurate error code...
    logFiles = glob.glob(os.path.join(logDir, "*"));
    logFile = logFiles[0];
    with open(logFile, 'r') as f:
        lines = f.read().splitlines()
        lastLine = lines[-1]
        regex = re.search(".+returning with error code: ", lastLine);
        if regex != None:
            codeStr = lastLine[regex.end():]
            code = int(codeStr);
    return [file, code, timeTaken, logDir]

def writeResults(file, results):
    fstream = open(file, "w");
    fstream.write("File,Exit Code,Time taken(s),Log location\n");
    for entry in results:
        fstream.write(entry[0] + "," + str(entry[1]) + ","+ str(entry[2]) + "," + entry[3]+"\n");
    fstream.close();

def getFileHash(file):
    md5 = hashlib.md5();
    block_size = 2**20;
    with open(file, "rb") as f:
        count = 0;
        while count < 10:
            data = f.read(block_size)
            if not data:
                break
            md5.update(data)
            count += 10;
    return md5.hexdigest()


if len(sys.argv) < 4:
    print "Usage: " + sys.argv[0] + " <extension> <file directory> <output file>"
    sys.exit(-1);

ext = sys.argv[1];
path = sys.argv[2];
outFile = sys.argv[3];

files = findAllFiles(path, ext);
files.sort()
results = [];
hashCheck = {};

count = 0;
logFolder = "log_" + datetime.now().strftime("%m-%d-%Y-%H_%M_%S");
os.mkdir(logFolder);
total = len(files);

for file in files:
    count+=1;
    md5 = getFileHash(file);
    print "["+str(count)+ "/" + str(total)+ "] " + file;
    if md5 in hashCheck:
        print "\t duplicated file, skipping..."
        results.append([file, "DUPLICATE", "", ""]);
        continue;
    hashCheck[md5] = 1;
    logPath = os.path.join(logFolder, str(count));
    results.append(runImportCmd(file, logPath));

writeResults(outFile, results);




