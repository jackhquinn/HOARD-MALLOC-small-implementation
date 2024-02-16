//
// Created by Clay Shields on 8/25/22.
//

#ifndef MALLOC_MAIN_H
#define MALLOC_MAIN_H

const int FUZZ_RUNS = 250000;
const int NUM_ALLOCS = 10;
const int MIN_ALLOC = 8;
const int MAX_ALLOC = 128 * 1024 ;
const int EXP_PARAM = 400;// Selected not at all carefully by just winging it
const double W_MIN = .5;
const double W_MAX = 7;
const char MIN_CHAR = '!';
const char MAX_CHAR = '~';

#endif //MALLOC_MAIN_H
