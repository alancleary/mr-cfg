/*
 * Copyright 2022 Alan Cleary
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDED_MR_CFG_FILE
#define INCLUDED_MR_CFG_FILE

#include <sdsl/construct.hpp>
#include <sdsl/int_vector.hpp>


namespace mr_cfg {


//! Loads text from a file into an int vector.
/*!
 *  \param filepath The path to the file to be loaded.
 *
 *  \return The int vector containing the text.
 */
sdsl::int_vector<8> load_text(const std::string& filepath) {

  // load the file into an int vector
  sdsl::int_vector<8> text;
  uint64_t num_bytes = 1;
  load_vector_from_file(text, filepath, num_bytes);

  return text;
}


}

#endif
