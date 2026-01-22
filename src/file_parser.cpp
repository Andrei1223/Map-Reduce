
#include "file_parser.h"

/* 
    convert from a normal word into a lowercase without any
    special characters
*/
std::string to_lower_case(std::string word)
{
    std::string new_word;

    // traverse each char in the word
    for (char c : word) {

        if ('A' <= c && c <= 'Z') {
            // convert from uppercase to lowercase
            c += ('a' - 'A');
        }

        if ('a' <= c && c <= 'z')
            new_word += c;
    }

    return new_word;
}

/*
    function that returns all the unique words from a given file
*/
void return_unique_words(std::string path, std::map<std::string, std::vector<int>> &map, int file_id)
{
    std::ifstream input(path);
    std::string word;

    // read each word from the list
    while (input >> word) {
        word = to_lower_case(word);


        // check if the file index hasnt been added(there cannot be duplicate index files)
        if (std::find(map[word].begin(), map[word].end(), file_id) == map[word].end()) {
            map[word].push_back(file_id);
        }
    }

    // close the file
    input.close();
}

/*
    function that reads the input file, that contains all the other file names
*/
void parse_input_file(std::string file_name, std::queue<std::string> &files)
{
    std::ifstream input(file_name);
    std::string word;

    // ignore the first line (it contains the total number of files)
    input >> word;

    // read each line in the file
    while (input >> word) {
        files.push(word);
    }
}
