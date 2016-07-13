//
//  execute.hpp
//  fsdiff-mr
//
//  Created by Brandon Abel Solis on 7/12/16.
//  Copyright © 2016 Learning Evironments. All rights reserved.
//

#ifndef execute_hpp
#define execute_hpp

#include <stdio.h>
#include <queue>
using namespace std;

vector<string> execute(string command, bool returnList);

const bool IGNORE_RESPONCE = false;
const bool RETURN_RESPONCE = true;

#endif /* execute_hpp */
