//
// Created by Clay Shields on 7/18/22.
//

#include "TestEntry.h"
TestEntry::TestEntry(){
    size = 0;
    address = nullptr;
    fill_char = MIN_CHAR;
}

char TestEntry::next_fill = 'A';

TestEntry::TestEntry(int new_size) {
    size = new_size;
    fill_char = next_fill;
    next_fill ++;
    if (next_fill > MAX_CHAR) next_fill = MIN_CHAR;
    address = nullptr;
}

bool TestEntry::allocate(unsigned char * addr) {
    if (addr == nullptr) {return false;}
    address = addr;
    return true;
}

void TestEntry::fill() {
    if (size != 0) {
        memset(address, fill_char, size);
    }
}

bool TestEntry::check() {
    if (size == 0) return true;

    unsigned char test[size];
    memset(test, fill_char, size);
    int val = memcmp(address, test, size);

    if (val != 0) return false;
    return true;
}

void TestEntry::empty() {
    if (address != nullptr) {
        memset(address, '\0', size);
    }
}

void TestEntry::sabotage() {
    //cout << "Sabotage entry at: " << (int *) address << " with len: " << size << endl;
    if ((address != nullptr) &&
        (size != 0))    {
        memset(address,'0',size);
    }

}

ostream &operator<<(ostream &os, const TestEntry &entry) {
    os << "fill_char: " << entry.fill_char << " size: " << entry.size << " address: " << entry.address;
    return os;
}


