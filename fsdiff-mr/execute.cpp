//
//  execute.cpp
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/12/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
//
#include <iostream>
#include <queue>
#include <stdio.h>

#include "execute.hpp"
#include "global.h"
using namespace std;



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