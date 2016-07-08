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
#include <queue>
#include <unistd.h> //for sleep
using namespace std;


bool v = false; //verbose
bool vv = false;//more verbose
const bool IGNORE_RESPONCE = false;
struct worker{
    string machine;
    bool busy;
};
vector<worker> workers;


//  Executes the command with an option to print the command and its responce
//Unsure if popen is necessary (as opposed to i.e execvp)
queue<string> execute(string command, bool returnList){
    if(vv) cout<<"  " << command<<endl;
    FILE *pipe = popen(command.c_str(), "r");
    if(pipe == NULL){
        perror("popen");
        exit(1);
    }
    
    char readbuf[80];
    queue<string> list;
    if(returnList){
        while(fgets(readbuf, 80, pipe)){
            list.push(readbuf);
        }
    } else if(vv){
        while(fgets(readbuf, 80, pipe)){
            cout<<"P: "<<readbuf<<endl;
        }
    }
    pclose(pipe);
    return list;
}

//Finds the trancript file
//copies it to master and restores the _original file
//assumption: the prossesed file made a foo.T an the original is stored as foo_original
void fetchAndRestore(string machine){
    string cmdSsh = "ssh "+ machine + " ";
    string cmdListFiles = "'ls /'";
    queue<string> files = execute(cmdSsh + cmdListFiles, true);
    
    string transcript;
    while(!files.empty()) {
        string file = files.front();
        if(file[file.size()-2] == 'T'){
            if(vv)cout<<"Found transcript: "<<file;
            //remove trailing \n
            transcript = file.substr(0, file.size()-1);
            break;
        }
        files.pop();
    }
    if(transcript.empty()){
        if(v) cout<<"Error: No transcript found in machine:"<<machine<<endl;
        return;
    }
    
    string cmdCopyTranscript = "scp "+machine+":/"+transcript+" /transcripts";
    string cmdRemoveTranscript = "'rm /" + transcript + "'";
    execute(cmdCopyTranscript, IGNORE_RESPONCE);
    execute(cmdSsh + cmdRemoveTranscript, IGNORE_RESPONCE);
    
    string file = transcript.substr(0, transcript.size()-2);//delete '.T'
    if(vv) cout<<"Restoring file: "<<file<<endl;
    string cmdRestorefile = "'mv /" + file + "_original /" + file + "'";
    execute(cmdSsh + cmdRestorefile, IGNORE_RESPONCE);
}
//checks if any worker machine is running fsdiff
//hangs until it finds one
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
            
            //the following may be unreliable. Better to parse the strings and analyze them
            //possibly pass in an echo or comment to the command to act as sentinel
            string cmdSsh = "ssh root@" + mworker.machine + " ";
            string cmdFsdiffCount = "'ps aux | grep fsdiff | wc -l'";
            
            queue<string> result = execute(cmdSsh + cmdFsdiffCount, true);
            if(stoi(result.front()) <= 2){
                if(vv) cout<<mworker.machine<<" done, fetching transcript and restoring file"<<endl;
                fetchAndRestore(mworker.machine);
                mworker.busy = false;
                return &mworker;
            }
        }
        if(v) cout<<"No worker is availbale, sleeping for 10 seconds"<<endl;
        sleep(10);
    }
}


//sends the worker machines files to fsdiff
//stores their transcript (as filename.T) when they are done
//and sends another file if there are any left
void manageWorkers(vector<string> machines){
    if(v) cout<<"Getting files at the root directory"<<endl;
    string cmdListFiles = "ls /"; //Should I secify the / directory?
    queue<string> files = execute(cmdListFiles, true);
    if(v) cout<<"Found "<<files.size()<<" files"<<endl;
    
    //build workers
    for(string machine : machines){
        worker mworker;
        mworker.machine = machine;
        mworker.busy = false;
        workers.push_back(mworker);
    }
    if(v) cout<<"Number of workers "<<workers.size()<<endl;


//  TODO: send each file to a worker and send the fsdiff command
   while(!files.empty()){
       //chop off the new line character
       string file(files.front().begin(), files.front().end()-1);
       
       worker *mworker = findAvailableWorker();
       cout<<"REFERENCE: "<<mworker<<endl;
       if(v) cout<<mworker->machine<<" available, sending it file: "<<file<<endl;
       
       //TODO: file may not exist in other machine (also may have _original already appended to it)
       string cmdSsh = "ssh " + mworker->machine + " ";
       string cmdSshf ="ssh -f " + mworker->machine + " ";
       string cmdRenameFile = "'mv /" + file + " /" + file + "_original'";
       //recursive copy maintins original creation dates
       string cmdCopyFile = "scp -rp /" + file + " root@"+ mworker->machine + ":/";
       //double check trailing & with a bigger file
       string cmdFsdiff = "'cd / ; ulimit -n 1024; /usr/local/bin/fsdiff -CIcsha1 -o "+ file + ".T ./" + file + "' &";

       execute(cmdSsh + cmdRenameFile, false);
       execute(cmdCopyFile, false);
       execute(cmdSshf + cmdFsdiff, false);
       files.pop();
       mworker->busy = true;
       findAvailableWorker();
 
       return;//testing, only do once
   }
    
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
        execute(cmdSsh + cmdRenameClient, IGNORE_RESPONCE);
        execute(cmdCopyClient, IGNORE_RESPONCE);

    }
    if(v)cout<<"Creating working directory /transcripts"<<endl;
    string cmdMakeWorkspace = "mkdir /transcripts";
    execute(cmdMakeWorkspace, IGNORE_RESPONCE);
    
    
    manageWorkers(machines);
    
    //cleanup
    for(string machine : machines){
        string cmdSsh = "ssh " + machine + " ";
        string cmdRestoreClient = "'mv /var/radmind/client_original /var/radmind/client'";
        string cmdDeleteWorkingDirectory = "rm -rf /transcripts";
        
        if(v) cout<<"Restoring original client folder on "<<machine<<endl;
        execute(cmdSsh + cmdRestoreClient, IGNORE_RESPONCE);
        execute(cmdDeleteWorkingDirectory, IGNORE_RESPONCE);
    }

    return 0;
}