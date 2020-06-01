import glob, os, shutil
import sys, re, hashlib
from datetime import datetime
from subprocess import Popen, PIPE
from threading import Timer


def isPass(code):
    return code == 0 or code == 7 or code == 10

def determineResult(file):
    with open(file, 'r') as f:
        lines = f.read().splitlines()
        for line in lines[1:]:
            data = line.split(",");
            if data[1] == "NOT RAN":
                break;
            if data[1] == "DUPLICATE":
                continue;
            if data[1] == "TIMED OUT":
                destination = os.path.join(os.path.join(os.path.dirname(data[0]), "Timedout"), os.path.basename(data[0]))
                shutil.move(data[0], destination);
            elif isPass(int(data[1])) :
                destination = os.path.join(os.path.join(os.path.dirname(data[0]), "Passed"), os.path.basename(data[0]))
                shutil.move(data[0], destination);
            else :
                destination = os.path.join(os.path.join(os.path.dirname(data[0]), "Failed"), os.path.basename(data[0]))
                shutil.move(data[0], destination);

if len(sys.argv) < 2:
    print "Usage: " + sys.argv[0] + " <csv file>"
    sys.exit(-1);

resFile = sys.argv[1];

determineResult(resFile);





