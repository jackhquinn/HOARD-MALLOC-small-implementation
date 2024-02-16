//
// Created by Clay Shields on 7/18/22.
//

#ifndef MALLOC_TESTENTRY_H
#define MALLOC_TESTENTRY_H
#include<random>
#include<iostream>
#include <cstring>
#include"main.h"
using namespace std;

class TestEntry {

public:

    // Constructor
    TestEntry();
    TestEntry(int size);

    // Methods
    bool allocate(unsigned char *);
    void fill();
    bool check();
    void empty();
    void sabotage();

    friend ostream &operator<<(ostream &os, const TestEntry &entry);

    // We will fill each address with a char to
    // see if there is any overwriting of allocated spaces
    static char next_fill;
    char fill_char;

    unsigned int size;

    // A pointer to the allocated space
    unsigned char * address;
};

#endif //MALLOC_TESTENTRY_H
