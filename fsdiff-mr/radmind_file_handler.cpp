//
//  radmind_filehandler.cpp
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/12/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
//

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
/*Appends '/dist_fsdiff' to every line in the /var/radmind/.../transcripts for safe copying*/
void processTrancripts(){
    execute("mkdir "+tmp_file, IGNORE_RESPONCE);
    string line;
    ifstream transcript_in("/var/radmind/client/applications/content-creation/joe-3_7.T");
    ofstream transcript_out("/tmp/joe-3_7.T");
    if(transcript_in.is_open() && transcript_out.is_open()){
        while(getline(transcript_in, line)){
            size_t dotPosition = line.find(".");
            line.insert(dotPosition +1 , tmp_file);
            transcript_out << line <<endl;
            
        }
        transcript_in.close();
        transcript_out.close();
    } else{
        if(!transcript_out.is_open()){
            cout<<"Because of oftream"<<endl;
        }
        if(!transcript_in.is_open()){
            cout<<"becuase of ifstream"<<endl;
        }
    }
    while(1);//hang here for testing
}

void setup(){
    processTrancripts();
    
    /*replace client folder in all target machines*/
    for(worker mworker: workers){
        string cmdSsh = "ssh " + mworker.machine + " ";
        string cmdRenameClient = "'mv /var/radmind/client /var/radmind/client_original'";
        string cmdCopyClient = "scp -r /var/radmind/client root@" + mworker.machine + ":/var/radmind/client";
        
        if(v) cout<<"Replacing /var/radmind/client of "<< mworker.machine <<endl;
        execute(cmdSsh + cmdRenameClient, IGNORE_RESPONCE);
        execute(cmdCopyClient, IGNORE_RESPONCE);
        
    }
    if(v)cout<<"Creating working directory /transcripts"<<endl;
    string cmdMakeWorkspace = "mkdir /transcripts";
    execute(cmdMakeWorkspace, IGNORE_RESPONCE);
}

void cleanup(){
    /*Restore client folder in target machines*/
    for(worker mworker : workers){
        string cmdSsh = "ssh " + mworker.machine + " ";
        string cmdRestoreClient = "'mv /var/radmind/client_original /var/radmind/client'";
        string cmdDeleteWorkingDirectory = "rm -rf /transcripts";
        
        if(v) cout<<"Restoring original client folder on "<<mworker.machine<<endl;
        execute(cmdSsh + cmdRestoreClient, IGNORE_RESPONCE);
        execute(cmdDeleteWorkingDirectory, IGNORE_RESPONCE);
    }
}