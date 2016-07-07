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
#include <stdio.h>
#include <vector>
using namespace std;


bool v = false; //verbose
bool vv = false;//more verbose

//  Executes the command with an option to print the command and its responce
//Unsure if popen is necessary (as opposed to i.e execvp)
vector<string> execute(string command, bool returnList){
    if(vv) cout<<"  " << command<<endl;
    FILE *pipe = popen(command.c_str(), "r");
    if(pipe == NULL){
        perror("popen");
        exit(1);
    }
    
    char readbuf[80];
    vector<string> list;
    if(returnList){
        while(fgets(readbuf, 80, pipe)){
            list.push_back(readbuf);
        }
    } else if(vv){
        while(fgets(readbuf, 80, pipe)){
            cout<<"P: "<<readbuf<<endl;
        }
    }
    pclose(pipe);
    return list;
}

//sends the worker machines files to fsdiff
//stores their transcript when they are done
//and sends another file if we are not done
void manageWorkers(vector<string> workers){
    string cmdListFiles = "ls /"; //Should I secify the directory?
    vector<string> files = execute(cmdListFiles, true);
    
    vector<pair<string, bool>> fileStatus;
    for(string file: files){
        fileStatus.push_back(make_pair(file, false));
    }
    if(v) cout<<"Found "<<fileStatus.size()<<" files"<<endl;
    
}


//TOOD: The authenticity of host... yes/no message
int main(int argc, const char * argv[]) {
    if(argc < 2) {
        cout<<"Please pass in at least one rdev-machine"<<endl;
        return -1;
    }
    //read flags
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
    //get list of rdev machines
    vector<string> machines;
    for(; i < argc; i++){
        machines.push_back(argv[i]);
    }
    
    //replace client
    for(string machine: machines){
        string cmdSsh = "ssh " + machine + " ";
        string cmdRenameClient = "'mv /var/radmind/client /var/radmind/client_original'";
        string cmdCopyClient = "scp -r /var/radmind/client root@" + machine + ":/var/radmind/client";
        
        if(v) cout<<"Replacing /var/radmind/client of "<< machine <<endl;
        execute(cmdSsh + cmdRenameClient, false);
        execute(cmdCopyClient, false);

    }
    
    manageWorkers(machines);
    
    //cleanup
    for(string machine : machines){
        string cmdSsh = "ssh " + machine + " ";
        string cmdRestoreClient = "'mv /var/radmind/client_original /var/radmind/client'";
        
        if(v) cout<<"Restoring original client folder on "<<machine<<endl;
        execute(cmdSsh + cmdRestoreClient, false);
    }

    return 0;
}