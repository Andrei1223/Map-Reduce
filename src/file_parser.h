# pragma once

#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <queue>
#include <algorithm>


void return_unique_words(std::string path, std::map<std::string, std::vector<int>> &map, int file_id);

void parse_input_file(std::string file_name, std::queue<std::string> &files);
