
/***********************************************************************

Copyright 2017 quantOS-org

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

***********************************************************************/
/* CSV_Reader
 * CSV_Reader does NOT store large amounts of data.
 * It works by reading a file, and finds the size of the data (rows + cols)
 * Data is accessed "on demand" so we don't need to store anything.
 *
 * Note: Currently all information is stored as std::string.
 */

#pragma once
#include <string>
#include <vector>

struct LocationIndex {
    int row;
    int col;
    bool valid;
};

class CSVReader
{
private:
    std::string file_loc_;
    int num_rows_;
    int num_cols_;
    bool perserve_double_quotes_;
    bool GenerateCols(std::string data);
public:

    std::vector< std::vector<std::string> > rows;

    CSVReader(std::string file_loc);
    ~CSVReader();
      bool LoadFile(bool perserve_double_quotes = false);
    int get_num_rows();
    int get_num_cols();
    LocationIndex FindString(std::string value, int start_row, int end_row, int start_col, int end_col);
    LocationIndex FindString(std::string value);
};
