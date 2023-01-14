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

#include <algorithm>  // min
#include <iostream>

#include <sdsl/construct.hpp>
#include <sdsl/csa_wt.hpp>

#include "mr-cfg/cfg.hpp"
#include "mr-cfg/file.hpp"
#include "mr-cfg/timer.hpp"

using namespace std;
using namespace sdsl;
using namespace mr_cfg;


void usage(int argc, char* argv[]) {
  cerr << "Usage: " << argv[0] << " {OPTIMAL|ONLINE|FAST} <FILE>" << endl;
}


int main(int argc, char* argv[])
{

  // check the command-line arguments
  if (argc < 3) {
    usage(argc, argv);
    return 1;
  }
  const string algorithm = argv[1];
  if (algorithm.compare("OPTIMAL") != 0 &&
      algorithm.compare("ONLINE") != 0 &&
      algorithm.compare("FAST") != 0)
  {
    usage(argc, argv);
    return 1;
  }

  // start timing
  Timer timer;

  // load the input file
  timer.startTask();
  cout << "loading file" << endl;
  const string filepath = argv[2];
  int_vector<8> text = load_text(filepath);
  timer.endTask();

  // construct the Compressed Suffix Array (Wavelet Tree of a Burrows-Wheeler
  // Transform)
  timer.startTask();
  cout << "building CSA" << endl;
  csa_wt<wt_huff<>> csa;
  construct_im(csa, text);
  typedef csa_wt<wt_huff<>>::size_type size_type;

  cout << "\tcsa size: " << csa.size() << endl;
  cout << "\talphabet: " << csa.sigma << endl;
  cout << "\twavelet tree size: " << csa.wavelet_tree.size() << endl;
  //cout << "bits size: " << run_bits.size() << endl;
  //cout << "i\tSA\tT[SA[i]..]" << endl;
  //for (size_type i = 0; i < csa.size(); ++i) {
  //  auto sa = csa[i];
  //  cout << i << "\t" << sa << "\t";
  //  while (sa < min(csa[i]+20, text.size())) {
  //    char c = text[sa];
  //    if (c == '\r' || c == '\n') {
  //      cout << "\\n";
  //    } else {
  //      cout << c;
  //    }
  //    sa += 1;
  //  }
  //  cout << endl;
  //}
  timer.endTask();

  // compute the context-free grammar
  timer.startTask();
  cout << "copmuting CFG" << endl;
  auto [cfg, start_rule] = csaToCfg(csa, algorithm);

  size_type total_size = csa.sigma;
  for (const auto& [rule, production] : cfg) {
    total_size += production.size();
  }
  cout << "\tnumber of rules: " << cfg.size() + csa.sigma << endl;
  cout << "\tstart rule size: " << cfg[start_rule].size() << endl;
  cout << "\ttotal non-start size: " << total_size - cfg[start_rule].size() << endl;
  cout << "\ttotal size: " << total_size << endl;
  timer.endTask();

  // regenerate the input file from the CFG for verification
  timer.startTask();
  cout << "printing CFG" << endl;
  printCfg(csa, cfg, start_rule);
  timer.endTask();

  return 1;
}
