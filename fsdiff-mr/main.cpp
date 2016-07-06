//
//  main.cpp
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/6/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
//

#include <iostream>
using namespace std;

int main(int argc, const char * argv[]) {
    string command = "ssh root@rdev-mm-51-im27 'ls'";
    int ret = system(command.c_str());
    if(ret != 0){
        cout<<"uh-oh"<<endl;
        return 0;
    }
    std::cout << "Hello, World!\n";
    return 0;
}
