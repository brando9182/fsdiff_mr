//
//  main.cpp
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/6/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
/*
 cd /afs/ir.stanford.edu/users/s/o/solis17/Documents/work/fsdiff-mr/DerivedData/fsdiff-mr/Build/Products/Debug
*/
//TODO: too scp applcationsL
//Maintain a string for the fille a machine is working on

#include <iostream>
#include <fstream>
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
    string file;
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

/*
 *Finds the trancript file
 *copies it to master and restores the _original file
 */
void fetchAndRestore(worker mworker){
    if(mworker.file == ""){
        cout<<"Error: "+ mworker.machine + "busy without a file"<<endl;
    }
    string transcript = mworker.file + ".T";
    
    string cmdSsh = "ssh root@" + mworker.machine + " ";
    string cmdCopyTranscript = "scp "+mworker.machine+":/"+transcript+" /transcripts";
    string cmdRemoveTranscript = "'rm /" + transcript + "'";
    execute(cmdCopyTranscript, IGNORE_RESPONCE);
    execute(cmdSsh + cmdRemoveTranscript, IGNORE_RESPONCE);
    
    if(vv) cout<<"Restoring file: "<<mworker.file<<endl;
    string cmdRestorefile = "'mv /" + mworker.file + "_original /" + mworker.file + "'";
    execute(cmdSsh + cmdRestorefile, IGNORE_RESPONCE);
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
            string cmdSsh = "ssh root@" + mworker.machine + " ";
            string cmdFsdiffCount = "'ps aux | grep fsdiff | wc -l'";
            
            queue<string> result = execute(cmdSsh + cmdFsdiffCount, true);
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
    string cmdListFiles = "ls /"; //Should I secify the / directory?
    queue<string> files = execute(cmdListFiles, true);
    if(v) cout<<"Found "<<files.size()<<" files"<<endl;
    

   while(!files.empty()){
       /*chop off the new line character*/
       string file(files.front().begin(), files.front().end()-1);
       
       worker *mworker = findAvailableWorker();
       if(v) cout<<mworker->machine<<" available, sending it file: "<<file<<endl;
       
       //TODO: file may not exist in other machine (also may have _original already appended to it)
       string cmdSsh = "ssh " + mworker->machine + " ";
       string cmdSshf ="ssh -f " + mworker->machine + " ";
       string cmdRenameFile = "'mv /" + file + " /" + file + "_original'";
       string cmdCopyFile = "scp -rp /" + file + " root@"+ mworker->machine + ":/";
       //double check trailing & with a bigger file
       string cmdFsdiff = "'cd / ; ulimit -n 1024; /usr/local/bin/fsdiff -CIcsha1 -o "+ file + ".T ./" + file + "' &";

       execute(cmdSsh + cmdRenameFile, false);
       execute(cmdCopyFile, false);
       execute(cmdSshf + cmdFsdiff, false);
       files.pop();
       mworker->busy = true;
       mworker->file = file;
   }
    
}

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

void fsdiff_mr(){
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
    
    
    manageWorkers();
    
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
    
    fsdiff_mr();

    return 0;
}