//
//  main.cpp
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/6/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
/*
 cd /afs/ir.stanford.edu/users/s/o/solis17/Documents/work/fsdiff-mr/DerivedData/fsdiff-mr/Build/Products/Debug
*/

#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <queue>
#include <unistd.h> //for sleep

#include "global.h"
#include "execute.hpp"
#include "radmind_file_handler.hpp"
using namespace std;

bool v = false; //verbose
bool vv = false;//more verbose
vector<worker> workers;


/*
 *Finds the trancript file
 *copies it to master and restores the _original file
 */
void fetchAndRestore(worker mworker){
    if(mworker.file == ""){
        cout<<"Error: "+ mworker.machine + "busy without a file"<<endl;
    }
    string transcript = mworker.file + ".T";
    
    command ssh = "ssh root@" + mworker.machine + " ";
    command copyTranscript = "scp "+mworker.machine+":/"+transcript+" /transcripts";
    command removeTranscript = "'rm /" + transcript + "'";
    execute(copyTranscript, IGNORE_RESPONCE);
    execute(ssh + removeTranscript, IGNORE_RESPONCE);
    
    if(vv) cout<<"Restoring file: "<<mworker.file<<endl;
    command restorefile = "'mv /" + mworker.file + "_original /" + mworker.file + "'";
    execute(ssh + restorefile, IGNORE_RESPONCE);
}


//Multithreading?
//pro: efficiency++
//con: complexity++
worker* findAvailableWorker(){
    while(true){
        for(auto &mworker : workers){
            if(!mworker.busy){
                if(vv)cout<<mworker.machine<<" not busy"<<endl;
                return &mworker;
            }
            
            /*query runnig programs to find fsdiff, note 2 false positives from this comand*/
            command ssh = "ssh root@" + mworker.machine + " ";
            command fsdiffCount = "'ps aux | grep fsdiff | wc -l'";
            
            vector<string> result = execute(ssh + fsdiffCount, true);
            if(stoi(result.front()) <= 2){
                if(vv) cout<<mworker.machine<<" done, fetching transcript and restoring file"<<endl;
                fetchAndRestore(mworker);
                mworker.busy = false;
                mworker.file = "";
                return &mworker;
            }
        }
        if(vv) cout<<"No worker is availbale, sleeping for 10 seconds"<<endl;
        sleep(10);
    }
}


//sends the worker machines files to fsdiff
//stores their transcript (as filename.T) when they are done
//and sends another file if there are any left
void manageWorkers(){
    if(v) cout<<"Getting files at the root directory"<<endl;
    command listFiles = "ls /"; //Should I secify the / directory?
    vector<string> files = execute(listFiles, true);
    if(v) cout<<"Found "<<files.size()<<" files"<<endl;
    

    for(string file : files){
        /*chop off the new line character*/
        file.pop_back();
        worker *mworker = findAvailableWorker();
        if(v) cout<<mworker->machine<<" available, sending it file: "<<file<<endl;
       
        //TODO: file may not exist in other machine (also may have _original already appended to it)
        command ssh = "ssh " + mworker->machine + " ";
        command sshf ="ssh -f " + mworker->machine + " ";
        command renameFile = "'mv /" + file + " /" + file + "_original'";
        command copyFile = "scp -rp /" + file + " root@"+ mworker->machine + ":/";
        //double check trailing & with a bigger file
        command fsdiff = "'cd / ; ulimit -n 1024; /usr/local/bin/fsdiff -CIcsha1 -o "+ file + ".T ./" + file + "' &";

        execute(ssh + renameFile, false);
        execute(copyFile, false);
        execute(sshf + fsdiff, false);
        mworker->busy = true;
        mworker->file = file;
   }
    
}


//TODO: The authenticity of host... yes/no message
int main(int argc, const char * argv[]) {
    if(argc < 2) {
        cout<<"Please pass in at least one rdev-machine"<<endl;
        return -1;
    }
    int i = 1;
    for(; i < argc && *argv[i] == '-'; i++){
        switch(argv[i][1]){
            case 'v': {
                v = true;
                if(strlen(argv[i]) > 1 && argv[i][2] == 'v'){
                    vv = true;
                }
            }
        }
    }
    for(; i < argc; i++){
        worker mworker;
        mworker.machine = argv[1];
        mworker.busy = false;
        workers.push_back(mworker);
    }
    if(v) cout<<"Number of workers "<<workers.size()<<endl;
    
    setup();
    manageWorkers();
    cleanup();


    return 0;
}