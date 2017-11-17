
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
#include "csv-reader.h"
#include <fstream>
#include <sstream>

CSVReader::CSVReader(std::string file_loc)
{
    file_loc_ = file_loc;
}

CSVReader::~CSVReader()
{

}

bool CSVReader::LoadFile(bool perserve_double_quotes)
{
    perserve_double_quotes_ = perserve_double_quotes;

    if (file_loc_ == "") {
        return false;
    } else {
        std::ifstream mainFile;
        mainFile.open(file_loc_.c_str());

        std::stringstream strStream;
        strStream << mainFile.rdbuf(); //read the file
        std::string data = strStream.str(); //str holds the content of the file

        GenerateCols(data);

        mainFile.close();
        num_rows_ = rows.size();
    }

    return true;
}

int CSVReader::get_num_rows()
{
    return this->num_rows_;
}

int CSVReader::get_num_cols()
{
    return this->num_cols_;
}

/*
 * This function locates the index of the *first* matching value container.
 * Searches rows between: start_row and end_row and
 * Columns between: start_col and end_col.
 *
 * Negatives values for the indices indicate no value. i.e. start_row == -1 implies start_row = 0 and
 * end_row == -1 implies end_row = LAST_ROW_INDEX
 */
LocationIndex CSVReader::FindString(std::string value, int start_row, int end_row, int start_col, int end_col)
{
    if (start_row < 0) {
        start_row = 0;
    }

    if (end_row < 0) {
        end_row = this->get_num_rows() - 1;
    }

    if (start_col < 0) {
        start_col = 0;
    }

    if (end_col < 0) {
        end_col = this->get_num_cols() - 1;
    }

    // Search

    LocationIndex index;
    index.valid = false;

    for (int i = start_row; i <= end_row; i++) {
        for (int j = start_col; j <= end_col; j++) {
            if (this->rows[i][j] == value) {
                index.row = i;
                index.col = j;
                index.valid = true;
            }
        }
    }

    return index;
}

LocationIndex CSVReader::FindString(std::string value)
{
    return FindString(value, -1, -1, -1, -1);
}

bool CSVReader::GenerateCols(std::string data)
{
    char DOUBLE_QUOTE_CHAR = 34;
    char COMMA_CHAR = 44;
    char END_LINE = '\n';

    std::vector< std::string > col;
    std::string currentCollected = "";
    bool escaping = false;

    for (unsigned int i = 0; i < data.length(); i++) {
      char currentChar = data.at(i);

      if ((currentChar != END_LINE && currentChar != DOUBLE_QUOTE_CHAR && currentChar != COMMA_CHAR)
          || (currentChar == COMMA_CHAR && escaping)
          || (currentChar == END_LINE && escaping)) {
        currentCollected += currentChar;
      } else if ((currentChar == END_LINE && !escaping)) {
        // End of row.
        // If we didn't write out data already, write out the remainder.
        col.push_back(currentCollected);
        num_cols_ = col.size();
        rows.push_back(col);
        col.clear();
        currentCollected = "";
      } else if (currentChar == COMMA_CHAR && !escaping) {
        // End of cell.
        col.push_back(currentCollected);
        currentCollected = "";
      } else if (currentChar == DOUBLE_QUOTE_CHAR && !escaping) {
        // We need to start escaping.
        escaping = true;

        currentCollected += currentChar;
      } else if (currentChar == DOUBLE_QUOTE_CHAR && escaping) {
        // If the next char is also a DOUBLE_QUOTE_CHAR, then we want to push a DOUBLE_QUOTE_CHAR.
        if (i < data.length() - 1 && data.at(i+1) == DOUBLE_QUOTE_CHAR) {
          currentCollected += currentChar;
          if (perserve_double_quotes_) {
            currentCollected += currentChar;
          }
          i++; // Skip next quote char.
        } else {
          // If not, then this is the end of the escaping period.
          escaping = false;
        
          currentCollected += currentChar;
        }
      }
    }

    if (col.size() != 0) {
      col.push_back(currentCollected);
      rows.push_back(col);
    }

    return true;
}
