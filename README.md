# APD - Homework 1

## Overview

This program implements a parallel inverted index using the Map-Reduce paradigm with Pthreads in C++. It processes a collection of text files and generates an inverted index. The index maps each unique word to the list of file IDs where it appears, grouped alphabetically into separate output files (a file for each letter, even though no word in the text starts with that letter).

## Execution Flow

    1. Input: A text file specifying the number of documents to process and their file names.
    2. Mapper Threads: Extract unique words and the files they appear in, creating partial results.
    3. Reducer Threads: Aggregate the partial results from Mappers, group words by their starting letter, and generate sorted output files.

## Functions and Their Roles

1) Main Program Functions

  • main(int argc, char **argv)

    • Purpose: Entry point of the program. It sets up the environment, initializes all the threads, and  manages execution.

    • Steps:
            1. Parses command-line arguments:
                • Number of Mapper threads.
                • Number of Reducer threads.
                • Input file name.
            2. Validates input parameters.
            3. Creates a shared structure (thread_common) for all threads.
            4. Starts all Mapper and Reducer threads using create_threads.
            5. Waits for all threads to finish using wait_for_threads.
            6. Frees allocated memory using free_arguments.


  • create_threads(int total_threads, int mapper_threads, thread_common &thread_common)

    • Purpose: Creates and launches Mapper and Reducer threads.

    • Parameters:
        • total_threads: Total number of threads (Mapper + Reducer).
        • mapper_threads: Number of Mapper threads.
        • thread_common: Shared data structure for thread communication.

    • Steps:
        1. Allocates thread_arg for each thread.
        2. Creates threads:
            • First mapper_threads as Mappers.
            • Remaining as Reducers.
        3. Returns a map of threads and their arguments.


  • wait_for_threads(std::map<pthread_t, thread_arg *> &threads)

    • Purpose:
        • Joins all threads, ensuring the program waits for their completion.


  • free_arguments(std::map<pthread_t, thread_arg *> &threads)

        •Purpose: Frees memory allocated for threads and the shared structure. Destroys all mutexes and condition variables.

2) Mapper Functions

  • void *mapper(void *aux)

    • Purpose: Executed by each Mapper thread to process files and extract unique words.

    • Steps:
        1. Calls next_file to fetch the next file to process.
        2. For each word in the file:
            • Converts it to lowercase using to_lower_case.
            • Adds it to a partial map of unique words ({word, [file ID]}).
        3. Marks the thread as finished using synchronization primitives.


  • void next_file(thread_arg &argv, std::string &file_name, int &file_id)

    • Purpose: Dynamically assigns the next file to a Mapper thread.

    • Steps:
        1. Locks a mutex to access the shared queue of files.
        2. Fetches the next file name and its ID if available.
        3. If no files remain, returns -1 to indicate completion.
        4. Unlocks the mutex.


  • std::string to_lower_case(std::string word)

    • Purpose: Converts a word to lowercase and removes non-alphabetic characters.

    • Parameters:
        • word: The input word.
    • Returns: A sanitized, lowercase version of the input word.


  • void return_unique_words(std::string path, std::map<std::string, std::vector<int>> &map, int file_id)

    • Purpose: Extracts unique words from a file and maps them to the current file ID.

    • Steps:
        1. Reads words from the file.
        2. Converts each word to lowercase.
        3. Ensures no duplicate file IDs are associated with a word.


    3) Reducer Functions

  • void *reducer(void *aux)

    • Purpose: Executed by each Reducer thread to merge Mapper results and write output files.

    • Steps:
        1. Waits for all Mapper threads to finish using condition variables.
        2. One Reducer merges Mapper results using merge_mapper_results.
        3. Each Reducer processes a portion of the merged results:
            • Groups words by their starting letter.
            • Writes the results to an output file using write_in_file.


  • void merge_mapper_results(vector<map<string, vector<int>>> &unique_words, vector<pair<char, vector<pair<string, vector<int>>>>> &result)

    • Purpose: Combines Mapper outputs and groups words by their starting letters.

    • Steps:
        1. Iterates through each Mapper's results.
        2. Aggregates words starting with the same letter.
        3. Ensures all letters of the alphabet are present, even if empty.


  • void write_in_file(thread_arg &argv, char letter, vector<pair<string, vector<int>>> &entry)

    • Purpose: Writes sorted word data to an output file for a given letter.

    • Steps:
        1. Constructs the output file name (<letter>.txt).
        2. Sorts words by:
            • Number of file IDs in descending order.
            • Alphabetically for ties.
            • Writes words and file IDs to the output file.


  • bool comparePairs(const pair<string, vector<int>> &a, const pair<string, vector<int>> &b)

    • Purpose: Comparator function for sorting word pairs.
    
    • Parameters: 2 word pairs to compare.

    • Returns: 
        • true if: a's file list is larger than b's.
        • alphabetically smaller when sizes are equal.


    4) Shared Structure and Utilities


  • thread_common *create_common_mapper_argument(std::string input_file, int num_mapper, int num_threads)

    • Purpose: Initializes the shared data structure for thread synchronization.

    • Steps:
        1. Reads the input file to populate the queue of file names.
        2. Initializes:
            • Counters for completed threads.
            • Unique word maps for Mappers.
            • Mutexes and condition variables.


  • void parse_input_file(std::string file_name, std::queue<std::string> &files)
    
    • Purpose: Reads the input file and enqueues the names of files to process.
