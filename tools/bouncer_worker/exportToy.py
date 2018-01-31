#usage: exportToy.py <dbAdd> <dbPort> <username> <password> <dbname> <file for list of model ids> <fed id>

import sys
import os

if len(sys.argv) < 8:
    print "Error: Not enough parameters."
    print "Usage: "+ sys.argv[0] +" <dbAdd> <dbPort> <username> <password> <dbname> <file for list of model ids>"
    sys.exit(1)

dbAdd = sys.argv[1]
dbPort = sys.argv[2]
dbUsername = sys.argv[3]
dbPassword = sys.argv[4]
dbName = sys.argv[5]
fileToModelIDs = sys.argv[6]
fedID = sys.argv[7]

#Find out the model IDs by reading the model ID file
fp = open(fileToModelIDs, "r")
modelList = fp.readlines()
fp.close()

colsInModel = ["groups", "history", "history.chunks", "history.files", "issues", "scene",
                "scene.files", "scene.chunks", "stash.3drepo", "stash.3drepo.chunks", "stash.3drepo.files",
                "stash.json_mpc.chunks", "stash.json_mpc.files", "stash.unity3d.chunks", "stash.unity3d.files"] 

for model in modelList:
    model = model.replace('\n', '')
    modelDirectory = "toy/" + model
    if not os.path.exists(modelDirectory):
        os.makedirs(modelDirectory)
    for ext in colsInModel:
        cmd =  "mongoexport /host:" + dbAdd + " /port:" + dbPort + " /username:" + dbUsername + " /password:" + dbPassword + " /authenticationDatabase:admin /db:" + dbName + " /collection:" + model + "." + ext  + " /out:" + modelDirectory + "/"  + ext + ".json";
        os.system(cmd)

#only export groups and issues from federation
colsInFed = ["groups", "issues"]

modelDirectory = "toy/" + fedID
if not os.path.exists(modelDirectory):
    os.makedirs(modelDirectory)
for ext in colsInFed:
        cmd =  "mongoexport /host:" + dbAdd + " /port:" + dbPort + " /username:" + dbUsername + " /password:" + dbPassword + " /authenticationDatabase:admin /db:" + dbName + " /collection:" + fedID + "." + ext  + " /out:" + modelDirectory + "/" + ext + ".json";  
        os.system(cmd)

