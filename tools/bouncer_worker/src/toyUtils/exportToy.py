#usage: exportToy.py <dbAdd> <dbPort> <username> <password> <dbname> <file for list of model ids> <fed id>

import sys
import os
from pymongo import MongoClient
import shutil
import re


if len(sys.argv) < 9:
    print "Error: Not enough parameters."
    print "Usage: "+ sys.argv[0] +" <dbAdd> <dbPort> <username> <password> <fileshare> <dbname> <file for list of model ids> <fed model id>"
    sys.exit(1)

dbAdd = sys.argv[1]
dbPort = sys.argv[2]
dbUsername = sys.argv[3]
dbPassword = sys.argv[4]
fileShareDir = sys.argv[5]
dbName = sys.argv[6]
fileToModelIDs = sys.argv[7]
fedID = sys.argv[8]

print "db add: " + dbAdd
print "db port: " + dbPort
print "db username: " + dbUsername
print "db pw: " + dbPassword
print "fileshare location: " + fileShareDir
print "db Name: " + dbName
print "model file: " + fileToModelIDs
print "fedId: " + fedID

from datetime import datetime

#Find out the model IDs by reading the model ID file
fp = open(fileToModelIDs, "r")
modelList = fp.readlines()
fp.close()

connString = "mongodb://"+ dbUsername + ":" + dbPassword +"@"+dbAdd + ":" + dbPort + "/"
db = MongoClient(connString)[dbName]
toyFolder = "toy_" + datetime.today().strftime('%Y-%m-%d');
toyFolderFullDir = os.path.join(fileShareDir , toyFolder);
if not os.path.exists(toyFolderFullDir):
    os.makedirs(toyFolderFullDir)

def  grabExternalFilesAndRewrite(collection):
    print "Copying files from file share to toy project folder : " + collection
    for entry in db[collection].find({"type": "fs"}):
        pathToFile = os.path.join(fileShareDir, entry["link"])
        try:
            shutil.copy(pathToFile, toyFolderFullDir)
        except:
            print "copy failed: " + pathToFile + ","+  toyFolderFullDir
        newPath = re.sub(r'.+/.+/', toyFolder + "/", entry["link"])
        newPath = re.sub(r'toy_.+/', toyFolder + "/", newPath)
        entry["link"] = newPath;
        entry["noDelete"] = True;
        db[collection].save(entry);
    return

def findModelCols(model):
    res = []
    for collection in  db.list_collection_names():
        if collection.startswith(model):
            res.append(collection)
    return res


for model in modelList:
    model = model.replace('\n', '')
    modelDirectory = "toy/" + model
    if not os.path.exists(modelDirectory):
        os.makedirs(modelDirectory)
    for col in findModelCols(model):
        if col.endswith(".ref"):
            grabExternalFilesAndRewrite(col)

        cmd =  "mongoexport /host:" + dbAdd + " /port:" + dbPort + " /username:" + dbUsername + " /password:" + dbPassword + " /authenticationDatabase:admin /db:" + dbName + " /collection:" + col  + " /out:" + modelDirectory + "/"  + col.replace(model +".", "", 1) + ".json";
        os.system(cmd)

#export groups, issues, risks, and views from federation
colsInFed = ["groups", "issues", "risks", "views"]

modelDirectory = "toy/" + fedID
if not os.path.exists(modelDirectory):
    os.makedirs(modelDirectory)
for ext in colsInFed:
        cmd =  "mongoexport /host:" + dbAdd + " /port:" + dbPort + " /username:" + dbUsername + " /password:" + dbPassword + " /authenticationDatabase:admin /db:" + dbName + " /collection:" + fedID + "." + ext  + " /out:" + modelDirectory + "/" + ext + ".json";
        os.system(cmd)

