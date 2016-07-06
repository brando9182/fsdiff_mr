//
//  main.cpp
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/6/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
//  cd /afs/ir.stanford.edu/users/s/o/solis17/Documents/work/fsdiff-mr/DerivedData/fsdiff-mr/Build/Products/Debug
//

#include <iostream>
#include <stdio.h>
using namespace std;

int main(int argc, const char * argv[]) {
    string command = "ssh root@rdev-mm-51-im27 'ls'";
    string command_test = "echo hello";
    FILE *pipein_ssh = popen(command_test.c_str(), "r");
    
    if(pipein_ssh == NULL){
        perror("popen");
        exit(1);
    }
    char readbuf[80];
    while(fgets(readbuf, 80, pipein_ssh))
        cout<<"Reading:"<<readbuf<<endl;
    pclose(pipein_ssh);
    
    std::cout << "Done reading\n";
    return 0;
}