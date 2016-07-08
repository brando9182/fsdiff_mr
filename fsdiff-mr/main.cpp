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

struct worker{
    string machine;
    bool busy;
};

//Finds the trancript file
//copies it to master and restores the _original file
void fetchAndRestore(string machine){
    cout<<"Entering fetch and retore"<<endl;
    string cmdListFiles = "ls /";
    queue<string> files = execute(cmdListFiles, true);
    while(!files.empty()) {
        string file = files.front();
        cout<<file<<", "<<file.length()<<" characters"<<endl;
        files.pop();
    }
    //find the .T file
    //copy it
    //restore old file
    
    string cmdSsh = "ssh " + machine + " ";
    //string cmdRestorefile = "'mv /" + file + "_original /" + file + "'";

    //copy transcript elsewhere
    //if(vv)cout<<"Restoring file:"<<file<<"on "<<mworker.machine<<endl;
    //execute(cmdSsh + cmdRestorefile, false);
}
//checks if any worker machine is running fsdiff
//hangs until it finds one
//Multithreading?
//pro: efficiency++
//con: complexity++
worker findAvailableWorker(vector<worker> workers){
    while(true){
        for(worker mworker : workers){
            if(!mworker.busy) return mworker;
            //the following may be unreliable. Better to parse the strings and analyze them
            //possibly pass in an echo or comment to the command to act as sentinel
            string cmdSsh = "ssh root@" + mworker.machine + " ";
            string cmdFsdiffCount = "'ps aux | grep fsdiff | wc -l'";
            
            queue<string> result = execute(cmdSsh + cmdFsdiffCount, true);
            if(stoi(result.front()) <= 2){
                cout<<"about to enter"<<endl;
                fetchAndRestore(mworker.machine);
                mworker.busy = false;
                return mworker;
            }
        }
        if(v) cout<<"No worker is availbale, sleeping for 10 seconds"<<endl;
        sleep(10);
    }
}

//ssh -f root@rdev-cl-41-imir "cd / ; ulimit -n 1024; /usr/local/bin/fsdiff -CIcsha1 ."

//sends the worker machines files to fsdiff
//stores their transcript (as filename.T) when they are done
//and sends another file if there are any left
void manageWorkers(vector<string> machines){
    //get files
    string cmdListFiles = "ls /"; //Should I secify the directory?
    queue<string> files = execute(cmdListFiles, true);
    if(v) cout<<"Found "<<files.size()<<" files"<<endl;
    
    //build workers
    vector<worker> workers;
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
       
       worker mworker = findAvailableWorker(workers);
       if(v) cout<<mworker.machine<<" availale, sending it file: "<<file<<endl;
       
       //TODO: file may not exist in other machine (also may have _original already appended to it)
       string cmdSsh = "ssh " + mworker.machine + " ";
       string cmdSshf ="ssh -f " + mworker.machine + " ";
       string cmdRenameFile = "'mv /" + file + " /" + file + "_original'";
       //recursive copy maintins original creation dates
       string cmdCopyFile = "scp -rp /" + file + " root@"+ mworker.machine + ":/";
       //double check trailing & with a bigger file
       string cmdFsdiff = "'cd / ; ulimit -n 1024; /usr/local/bin/fsdiff -CIcsha1 -o "+ file + ".T ./" + file + "' &";

       execute(cmdSsh + cmdRenameFile, false);
       execute(cmdCopyFile, false);
       //execute(cmdSshf + cmdFsdiff, false);
       files.pop();
       mworker.busy = true;
       
 
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