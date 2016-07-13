//
//  radmind_filehandler.cpp
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/12/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
//
// In order to produce transcripts on the worker identical to what they would've been on the master,
// handles the radmind overhead to make fsdiff think the workers are the original machine

#include <iostream>
#include <fstream>

#include "radmind_file_handler.hpp"
#include "execute.hpp"
#include "global.h"

#include <stdio.h>
#include <string>
using namespace std;

//TODO: ls -R get all transcripts and build new client with our tmp appended
string tmp_file = "/tmp/dist_fsdiff";
string clientFolder = "/var/radmind/client/";


/*  Appends a tmp nameholder to all transcripts */
void processTranscript(string transcript){
    string line;
    ifstream transcript_in(transcript);
    ofstream transcript_out("/tmp" + transcript);
    if(transcript_in.is_open()){
        if(transcript_out.is_open()){
            while(getline(transcript_in, line)){
                size_t dotPosition = line.find(".");
                line.insert(dotPosition +1 , tmp_file);
                transcript_out << line <<endl;
                
            }
            transcript_in.close();
            transcript_out.close();
        } else {
            cout<<"Failed to output to /tmp"<<transcript<<endl;
        }
    } else{
        cout<<transcript<<" failed to open"<<endl;

    }
    while(1);//hang here for testing
}
//todo: just copy entire folder to temp
void createNewTranscripts(){
    command copyClient = "cp "+clientFolder+ tmp_file;
    
    execute("mkdir "+tmp_file, IGNORE_RESPONCE);
    if(v) cout<<"Listing files under "<<clientFolder<<endl;
    string cmdListDirectories = "find " + clientFolder + " -name '*.T'";
    vector<string> paths = execute(cmdListDirectories, RETURN_RESPONCE);
    cout<<paths.size()<<endl;
    for(string path : paths){
        path.pop_back();
        processTranscript(path);
    }
    
    return;
}

/*  
 *  copies client folder to worker machines
 *  and creates transcript folder
 */
void setup(){
    createNewTranscripts();
    
    /*replace client folder in all target machines*/
    for(worker mworker: workers){
        command ssh = "ssh " + mworker.machine + " ";
        command renameClient = "'mv /var/radmind/client /var/radmind/client_original'";
        command copyClient = "scp -r /var/radmind/client root@" + mworker.machine + ":/var/radmind/client";
        
        if(v) cout<<"Replacing /var/radmind/client of "<< mworker.machine <<endl;
        execute(ssh + renameClient, IGNORE_RESPONCE);
        execute(copyClient, IGNORE_RESPONCE);
        
    }
    if(v)cout<<"Creating working directory /transcripts"<<endl;
    command makeWorkspace = "mkdir /transcripts";
    execute(makeWorkspace, IGNORE_RESPONCE);
}

/*  restores previous client folder on worker */
void cleanup(){
    for(worker mworker : workers){
        command ssh = "ssh " + mworker.machine + " ";
        command restoreClient = "'mv /var/radmind/client_original /var/radmind/client'";
        command deleteWorkingDirectory = "rm -rf /transcripts";
        
        if(v) cout<<"Restoring original client folder on "<<mworker.machine<<endl;
        execute(ssh + restoreClient, IGNORE_RESPONCE);
        execute(deleteWorkingDirectory, IGNORE_RESPONCE);
    }
}