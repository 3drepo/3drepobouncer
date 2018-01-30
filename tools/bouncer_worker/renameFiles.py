#usage: exportToy.py <file for list of model ids> <fed id>

import sys
import os
from os import walk

if len(sys.argv) < 3:
    print "Error: Not enough parameters."
    print "Usage: "+ sys.argv[0] +" <dbAdd> <dbPort> <username> <password> <dbname> <file for list of model ids>"
    sys.exit(1)

fileToModelIDs = sys.argv[1]
fedID = sys.argv[2]

#Find out the model IDs by reading the model ID file
fp = open(fileToModelIDs, "r")
modelList = fp.readlines()
fp.close()

for model in modelList:
    model = model.replace('\n', '')
    modelDirectory = "toy/" + model + "/Laika0115"
    for (dirpath, dirnames, filenames) in walk(modelDirectory):
        for filename in filenames:
            print "Before: " + filename + " after: " +  filename.replace(model + ".", "").replace("Laika0115/","")
            os.rename(modelDirectory+ "/" + filename, modelDirectory + "/" + filename.replace(model + ".", "").replace("Laika0115/",""))

modelDirectory = "toy/" + fedID  + "/Laika0115"
print "fed directory: " + modelDirectory
for (dirpath, dirnames, filenames) in walk(modelDirectory):
    for filename in filenames:
            print "Before: " + filename + " after: " +  filename.replace(fedID + ".", "").replace("Laika0115/","")
            os.rename(modelDirectory+ "/" + filename, modelDirectory + "/" + filename.replace(fedID + ".", "").replace("Laika0115/",""))

