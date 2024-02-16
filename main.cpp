#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <set>
#include <random>
#include "main.h"
#include "malloc.h"
#include "TestEntry.h"
using namespace std;

#define FUZZ_RUNS 1000000

void print_help(char* name) {
    cout << name << "[-h] [seed] [allocations] [fuzz size]] [min_alloc] [max_alloc]" << endl;
    cout << "  -h prints this message" << endl;
    cout << "  [seed] is optional and is the random seed to use, random if not specified" << endl;
    cout << "  [allocations] is optional and is the number of allocations to make, default: " << NUM_ALLOCS << endl;
    cout << "  [fuzz runs] is optional and is the number of operations to run, default: " <<  FUZZ_RUNS << endl;
    cout << "  [min_alloc] is optional min generated allocation size, default: " << MIN_ALLOC << endl;
    cout << "  [max_alloc] is optional max generated allocation size, default: " << MAX_ALLOC << endl;
    exit(0);
}

int main(int argc, char ** argv) {

    // Set up some random number generators so we can run repeatable experiments by
    // by specifying a seed

    random_device seed_generator;
    double seed = seed_generator();

    int min_allocation = MIN_ALLOC;
    int max_allocation = 2048;
    int run_allocs = NUM_ALLOCS;
    int fuzz_runs = FUZZ_RUNS;

    if (argc > 1) {
        // argc at least 2
        if (!strcmp(argv[1], "-h")) {
            print_help(argv[0]);
        } else {
            try {
                seed = stoi(argv[1]);
                cout << "Seed set to:" << seed << endl;
            }
            catch (std::exception const &e) {
                cout << "Seed error " << e.what() << endl;
                print_help(argv[0]);
                exit(0);
            }
        }
    }

    if (argc > 2) {
        try {
            run_allocs = stoi(argv[2]);
            cout << "Num allocs set to: " << run_allocs << endl;
        }
        catch (std::exception const &e) {
            cout << "Num allocs error:  " << e.what() << endl;
            print_help(argv[0]);
            exit(0);
        }
    }

    if (argc > 3) {
        try {
            fuzz_runs = stoi(argv[3]);
            cout << "Fuzz runs set to: " << fuzz_runs << endl;
        }
        catch (std::exception const &e) {
            cout << "Fuzz runs error:  " << e.what() << endl;
            print_help(argv[0]);
            exit(0);
        }
    }


    if (argc > 4) {
        try {
            min_allocation = stoi(argv[4]);
            cout << "Min alloc set to: " << min_allocation << endl;
        }
        catch (std::exception const &e) {
            cout << "Min address error " << e.what() << endl;
            print_help(argv[0]);
            exit(0);
        }
    }

    if (argc > 5) {
        try {
            max_allocation = stoi(argv[5]);
            cout << "Max alloc set to: " << max_allocation << endl;
        }
        catch (std::exception const &e) {
            cout << "Max address error " << e.what() << endl;
            print_help(argv[0]);
            exit(0);
        }
    }

    mt19937 mersenne_generator(seed);
    weibull_distribution<double> dist(W_MIN,W_MAX);

    cout << endl << endl << "==> Allocation tests" << endl;
    // Run some tests on the memory allocator
    vector<TestEntry> allocs;

    for (int i = 0; i < run_allocs; i++) {
        // allocate a bunch of random entries
        int t_size = dist(mersenne_generator) * 100;
        if (t_size < min_allocation) t_size = min_allocation;
        if (t_size > max_allocation) t_size = max_allocation;

        TestEntry x(t_size);

        if (!x.allocate((unsigned char *) mmalloc(x.size))){
            cout << "Allocation failed" << endl;
            break;
        }
        else{
            //cout << "Allocation num: " << i << " size: " << x.size << endl;
        }

        x.fill();
        allocs.push_back(x);
    }

    // This will check each allocation and see if it was somehow overwritten.
    // This should not print out anything if all is well
    for (int i = 0; i < allocs.size(); i++) {
        if (!allocs.at(i).check()) cout << "ERROR: Allocation overwritten" << endl;
    }

    mstats();

    cout << endl << endl << endl;

    cout << "==> Sabotage testing" << endl;
    // Let's sabotage about 10% of the allocations and see if they mess up others
    int num_sabotage = run_allocs/10;
    uniform_int_distribution<int> int_rand(0, run_allocs -1);
    set<int> sabotaged;
    for (int i = 0; i < num_sabotage; i++){
        int sabotage = int_rand(mersenne_generator);
        allocs.at(sabotage).sabotage();
        sabotaged.insert(sabotage);
    }

    for (int i = 0; i < allocs.size(); i++) {
        if (!allocs.at(i).check()) {
            if (sabotaged.count(i) == 0) {
                cout << "Unsabotaged entry found to contain error" << endl;
            }
            else {
                //cout << "Caught sabotage at: " << i << endl;
            }
        }
    }



    cout << endl << "Deallocating" << endl;
    for (int i = 0; i < allocs.size(); i++) {
        //cout << "Dealloc: " << (int *) allocs.at(i).address << endl;
        mfree(allocs.at(i).address);
    }

    mstats();
    cout << endl;

    //mprint();
    // Lets run some random allocations for a while and see if we find anything wrong;
    // This is essentially fuzzing

    cout << "==> Fuzzing" << endl;
    allocs.clear();

    uniform_int_distribution<int> another_int_rand(0, 10);
    for (int i = 0; i < fuzz_runs; i++){
        int opt = another_int_rand(mersenne_generator);
        if (opt < 6) {// Allocate somethings
            //cout << "Allocating" << endl;
            int t_size = dist(mersenne_generator) * 100;
            if (t_size < min_allocation) t_size = min_allocation;
            if (t_size > max_allocation) t_size = max_allocation;

            TestEntry x(t_size);
            if (!x.allocate((unsigned char *) mmalloc(x.size))) {
                cout << "Allocation failed" << endl;
                break;
            }

            x.fill();
            allocs.push_back(x);
        }
        else if (opt < 9) {
            //cout << "Deallocating" << endl;
            if (allocs.size() > 0 ) {// Deallocate something
                uniform_int_distribution<int> rand_alloc_gen(0, allocs.size() - 1);
                int rand_alloc = rand_alloc_gen(mersenne_generator);
                mfree(allocs.at(rand_alloc).address);
                allocs.erase(allocs.begin() + rand_alloc);
            }
        }
        else {
            //cout << "Testing" << endl;
            // test everything for errors
            for (int i = 0; i < allocs.size(); i++) {
                if (!allocs.at(i).check()) cout << "ERROR: Allocation overwritten" << allocs.at(i) << endl;
            }
        }
    }

    mstats();
    cout << endl;


    return 0;
}

/*
int main() {
    // Test with different sizes to check if there's any specific size causing issues
    const size_t testSizes[] = {16, 32, 64, 128, 256, 512, 1024, 1500, 2048};

    for (size_t testSize : testSizes) {
        cout << "Testing with size: " << testSize << " bytes" << endl;

        // Allocate memory using mmalloc
        void* ptr = mmalloc(testSize);
        if (ptr == nullptr) {
            cout << "mmalloc failed for size " << testSize << endl;
            continue;
        } else {
            cout << "Memory allocated at address: " << ptr << endl;
        }

        // Write to the allocated memory. This step will help ensure that the memory is valid.
        // This may cause a segfault if there's something wrong with the allocation.
        memset(ptr, 'A', testSize);

        // Deallocate memory using mfree
        mfree(ptr);
        cout << "Memory freed for size: " << testSize << endl;

        cout << "------------------------------------" << endl;
    }

    return 0;
}*/