
#pragma once

#include <queue>
#include <map>
#include <iostream>
#include <pthread.h>
#include <algorithm>

#include "file_parser.h"

using namespace std;

struct thread_common {
        // all the files that must be processed by the threads
        queue<string> *all_files;
        int *current_file_id;
        int num_mapper_threads, num_total_threads;

        // each mapper thread has a map of a string and the indexes
        vector<map<string, vector<int>>> *unique_words;
        vector<pair<char, vector<pair<string, vector<int>>>>> *merged_words;

        // array that stores if the thread at the i-th index has finished work or not
        int *num_finised_mapper;
        int *num_finised_reducer;

        /*
            synchronization variables
        */

        pthread_mutex_t mutex_all_files;
        pthread_mutex_t mutex_threads_finished;
        pthread_mutex_t mutex_merged_words;
        // condition variable used to signal all the threads that the work is finished
        pthread_cond_t is_work;
};


struct thread_arg {
    /*
        arguments that are the same for all the threads
    */
    struct thread_common &common;


    /*
        arguments that are unique to each thread
    */
    int thread_id;

    /*
        constructors
    */
    thread_arg(struct thread_common &argv_common) : common(argv_common) {}
};


/*
    function to create the shared structure for all mapper threads
*/
thread_common *create_common_mapper_argument(string input_file, int num_mapper, int num_threads);

/*
    function to create mapper and reducer threads
*/
map<pthread_t, thread_arg *> create_threads(int num_threads, int mapper_threads, thread_common &thread_common);

/*
    function to wait for all mapper threads to finish
*/
void wait_for_threads(map<pthread_t, thread_arg *> &threads);

/*
    function to free allocated memory for threads
*/
void free_arguments(map<pthread_t, thread_arg *> &threads);
