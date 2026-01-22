
#include "thread.h"

/*
    function to get the next file from the queue
    ensures only one thread can access it at a time
*/
void next_file(thread_arg &argv, std::string &file_name, int &file_id)
{
    // lock the mutex so that only one thread can get a file
    pthread_mutex_lock(&argv.common.mutex_all_files);

    // remove the element
    if (!argv.common.all_files->empty()) {
        // get the filename at the top of the queue
        file_name = argv.common.all_files->front();
        argv.common.all_files->pop();
    } else {
        file_id = -1;
        pthread_mutex_unlock(&argv.common.mutex_all_files);
        return ;
    }

    // get the file id
    file_id = *argv.common.current_file_id;

    (*argv.common.current_file_id)++;

    pthread_mutex_unlock(&argv.common.mutex_all_files);
}

/*
    function that is used to combine the output of each mapper thread, and split
    them by the starting letter of each word
*/
void merge_mapper_results(vector<map<string, vector<int>>> &unique_words,
                          vector<pair<char, vector<pair<string, vector<int>>>>> &result)
{
    // create a temporary map to organize words by their starting letter
    map<char, vector<pair<string, vector<int>>>> temp_result;

    // traverse the vector of maps
    for (const auto &word_map : unique_words) {
        for (const auto &entry : word_map) {
            const string &word = entry.first;
            const vector<int> &indices = entry.second;

            // ensure the word is non-empty and take its first letter
            if (!word.empty()) {
                char first_letter = word[0];

                // check if the word already exists
                auto &aux = temp_result[first_letter];
                auto poz = std::find_if(aux.begin(), aux.end(),
                        [&](const std::pair<std::string, std::vector<int>> &pair) { 
                            return pair.first == word; 
                        });

                if (poz == aux.end()) {
                    aux.emplace_back(word, indices);
                } else {
                    // the word exists so combine the indexes
                    poz->second.insert(poz->second.end(), indices.begin(), indices.end());
                }
            }
        }
    }

    // add the other empty letters
    for (char l = 'a'; l <= 'z'; l++) {
        // if the letter doesn t appear in the map add the entry with no input
        if (temp_result.find(l) == temp_result.end()) {
            temp_result.insert({l, {}});
        }
    }

    // convert the map to the required vector result format
    result.clear();
    for (auto &entry : temp_result) {
        result.emplace_back(entry.first, std::move(entry.second));
    }
}

/*
    function to compare pairs for sorting
    sorts by the size of the second element
    if equal sorts alphabetically by the first element
*/
bool comparePairs(const pair<string, vector<int>> &a, const pair<string, vector<int>> &b)
{
    // compare by the size of the vector
    if (a.second.size() != b.second.size())
        return a.second.size() > b.second.size();

    // if sizes are equal compare alphabetically by the string
    return a.first < b.first;
}

/*
    function to write the output for a specific letter
    writes the sorted words and their occurrences into a file
*/
void write_in_file(thread_arg &argv, char letter, vector<pair<string, vector<int>>> &entry)
{
    // construct the output file name
    string filename = string(1, letter) + ".txt";
    
    // open the file for writing
    ofstream output_file(filename, ios::out);

    // sort the words by order and if equal by number of entries
    sort(entry.begin(), entry.end(), comparePairs);

    // write each entry to the file
    for (auto &pair : entry) {
        // write the word
        output_file << pair.first << ":";

        // first sort the array with the files id
        auto &indexes = pair.second;

        sort(indexes.begin(), indexes.end());
        
        // write the indices
        output_file << "[";
        for (size_t i = 0; i < indexes.size(); ++i) {
            output_file << indexes[i];
            if (i < indexes.size() - 1) {
                output_file << " ";
            }
        }
        output_file << "]" << std::endl;
    }
    
    // close the file
    output_file.close();
}

/*
    function executed by mapper threads
    reads files and creates a map of unique words
*/
void *mapper(void *aux)
{
    thread_arg *mapper = (thread_arg *) aux;

    std::string file;
    int file_id, thread_id = mapper->thread_id;

    /*
        STEP 1 : read from the file and create the map of unique words
    */
    while (1) {
        // get the next file to read from
        next_file(*mapper, file, file_id);

        // if all the files have been processed the thread must wait
        if (file_id == -1) {
            // exit the main loop
            break;
        } else {
            // process the file
            return_unique_words(file, mapper->common.unique_words->at(thread_id), file_id);
        }
    }

    /*
        STEP 2 : mark the thread as finished
    */

    pthread_mutex_lock(&mapper->common.mutex_threads_finished);

    // set that this thread has finised work
    (*mapper->common.num_finised_mapper)++;

    // notify all the other waiting threads to check if the work is done
    pthread_cond_broadcast(&mapper->common.is_work);

    //std::cout << mapper->common.unique_words->at(thread_id).size() << "\n";

    pthread_mutex_unlock(&mapper->common.mutex_threads_finished);

    return NULL;
}

/*
    function executed by reducer threads
    combines results and writes the output
*/
void *reducer(void *aux)
{
    thread_arg *reducer = (thread_arg *) aux;
    int thread_id = reducer->thread_id;

    /*
        STEP 1 : wait for the mapper threads to finish work
    */

    // check if mapper threads have finised
    pthread_mutex_lock(&reducer->common.mutex_threads_finished);

    while (1) {

        // exit the loop if all the threads have finished
        if (*reducer->common.num_finised_mapper == reducer->common.num_mapper_threads) {
            // notify all the other threads to check
            pthread_cond_broadcast(&reducer->common.is_work);
            break;
        }

        pthread_cond_wait(&reducer->common.is_work, &reducer->common.mutex_threads_finished);       
    }

    pthread_mutex_unlock(&reducer->common.mutex_threads_finished);


    /*
        STEP 2 : only one reducer thread merges the results from the mapper threads
    */

    pthread_mutex_lock(&reducer->common.mutex_threads_finished);

    // check if the output from the mapper threads has been merged and sorted
    while (reducer->common.merged_words == nullptr) {
        // create the final map into which to combine the outputs
        reducer->common.merged_words = new vector<pair<char, vector<pair<string, vector<int>>>>>;

        merge_mapper_results(*reducer->common.unique_words, *reducer->common.merged_words);
    }

    pthread_mutex_unlock(&reducer->common.mutex_threads_finished);


    /*
        STEP 3 : each reducer thread gets an entry and writes it into the output file
    */

    bool finised = false;

    pair<char, vector<pair<string, vector<int>>>> entry;

    while (finised == false) {

        // get the next entry from the final map
        pthread_mutex_lock(&reducer->common.mutex_merged_words);

        if (reducer->common.merged_words->empty()) {
            finised = true;
            pthread_mutex_unlock(&reducer->common.mutex_merged_words);
            break;
        } else {
            int size = reducer->common.merged_words->size();
            // get a letter
            entry = reducer->common.merged_words->at(size - 1);

            // remove the letetr from the array
            reducer->common.merged_words->erase(reducer->common.merged_words->begin() + size - 1);
        }

        pthread_mutex_unlock(&reducer->common.mutex_merged_words);

        // write the words in the output file
        write_in_file(*reducer, entry.first, entry.second);
    }

    // wait for all the other threads to finish
    pthread_mutex_lock(&reducer->common.mutex_threads_finished);

    (*reducer->common.num_finised_reducer)++;

    pthread_cond_broadcast(&reducer->common.is_work);

    /*
        STEP 4 : make the threads to wait for all the other to finish work
    */

    while (1) {
        // check if all the threads have finished
        if (*reducer->common.num_finised_reducer + *reducer->common.num_finised_mapper == reducer->common.num_total_threads) {
            pthread_cond_broadcast(&reducer->common.is_work);
            break;
        }

        // wait for all the threads to finish
        pthread_cond_wait(&reducer->common.is_work, &reducer->common.mutex_threads_finished);
    }

    pthread_mutex_unlock(&reducer->common.mutex_threads_finished);

    return NULL;
}

/*
    functiom that creates the common element in the mapper arguments
*/
thread_common *create_common_mapper_argument(std::string input_file, int num_mapper, int num_threads)
{
    // read the file names from the input file
    std::queue<std::string> *files = new std::queue<std::string>;

    parse_input_file(input_file, *files);

    thread_common *mapper = new thread_common;

    mapper->all_files = files;
    mapper->current_file_id = new int(1);
    mapper->num_finised_mapper = new int(0);
    mapper->num_finised_reducer = new int(0);
    mapper->unique_words = new std::vector<std::map<std::string, std::vector<int>>>(num_mapper);
    mapper->merged_words = nullptr;

    mapper->num_mapper_threads = num_mapper;
    mapper->num_total_threads = num_threads;

    *mapper->current_file_id = 1;
    *mapper->num_finised_mapper = 0;
    *mapper->num_finised_reducer = 0;

    pthread_mutex_init(&mapper->mutex_all_files, NULL);
    pthread_mutex_init(&mapper->mutex_threads_finished, NULL);
    pthread_mutex_init(&mapper->mutex_merged_words, NULL);
    pthread_cond_init(&mapper->is_work, NULL);

    return mapper;
}


/*
    function thath creates all the threads
*/
std::map<pthread_t, thread_arg *> create_threads(int total_threads, int mapper_threads, thread_common &thread_common)
{
    // for each thread allocate memory for a new argument
    std::map<pthread_t, thread_arg *> threads;
    pthread_t aux;

    for (int i = 0; i < total_threads; i++) {
        thread_arg* argv = new thread_arg(thread_common);
        argv->thread_id = i;

        // create the mapper threads
        if (i < mapper_threads) {
            // create the thread and pass a pointer to the argument struct
            pthread_create(&aux, NULL, mapper, (void*)argv);
        } else { // create the reducer threads
            // create the thread and pass a pointer to the argument struct
            pthread_create(&aux, NULL, reducer, (void*)argv);
        }

        // save the thread and its header
        threads.insert({aux, argv});
    }

    return threads;
}

/*
    function that makes all the threads wait
*/
void wait_for_threads(std::map<pthread_t, thread_arg *> &threads)
{
    for (const auto &pair : threads) {
        // get the thread id
        pthread_t thread = pair.first;

        pthread_join(thread, NULL);
    }
}

/*
    function that frees the allocated memory for each thread argument
*/
void free_arguments(std::map<pthread_t, thread_arg *> &threads)
{
    bool deleted_common = false;
    for (auto &pair : threads) {
        auto arg = pair.second;

        // delete the common structure only once (because its the same variable)
        if (deleted_common ==  false) {
            delete arg->common.all_files;
            delete arg->common.current_file_id;
            delete arg->common.unique_words;
            delete arg->common.merged_words;
            delete arg->common.num_finised_mapper;
            delete arg->common.num_finised_reducer;

            pthread_mutex_destroy(&arg->common.mutex_all_files);
            pthread_mutex_destroy(&arg->common.mutex_threads_finished);
            pthread_mutex_destroy(&arg->common.mutex_merged_words);
            pthread_cond_destroy(&arg->common.is_work);

            deleted_common = true;
        }

        delete arg;

        pair.second = nullptr;
    }
}
