//
//  global.h
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/12/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
//

#ifndef global_h
#define global_h


using namespace std;

typedef string command;

struct worker{
    string machine;
    bool busy;
    string file;
};
extern vector<worker> workers;

extern bool v;
extern bool vv;

#endif /* global_h */
