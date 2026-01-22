
#include <iostream>
#include <map>

#include "file_parser.h"
#include "thread.h"

int main(int argc, char **argv)
{
    int num_mapper_threads, num_reducer_threads;
    std::string input_file;

    // get the input values
    num_mapper_threads = atoi(argv[1]);
    num_reducer_threads = atoi(argv[2]);
    input_file = std::string(argv[3]);

    if (num_mapper_threads <= 0) {
        std::cout << "Invalid number of mapper threads: " << num_mapper_threads << "\n";
        return -1;
    }

    if (num_reducer_threads <= 0) {
        std::cout << "Invalid number of reducer threads: " << num_reducer_threads << "\n";
        return -1;
    }

    // create the common argument for the mapper threads
    thread_common *thread_argv = create_common_mapper_argument(input_file, num_mapper_threads, num_mapper_threads + num_reducer_threads);

    // create the threads
    auto threads = create_threads(num_mapper_threads + num_reducer_threads, num_mapper_threads, *thread_argv);

    // wait for all the threads
    wait_for_threads(threads);

    free_arguments(threads);

    return 0;
}
