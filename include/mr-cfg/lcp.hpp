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

#ifndef INCLUDED_MR_CFG_LCP
#define INCLUDED_MR_CFG_LCP

#include <queue>
#include <unordered_set>
#include <vector>

#include <sdsl/bit_vectors.hpp>

#include "mr-cfg/generator.hpp"


namespace mr_cfg {


//! An implementation of the algorithm from "Space-Efficient Computation of
//  Maximal and Supermaximal Repeats in Genome Sequences" by Beller, et al.;
//  computes all LCP-intervals of a string given using an FM-index.
//
//  LCP-intervals are computed in length-lexicographical order.
//
//  O(n\log\sigma), where n is the length of the string and \sigma is the size
//  of the alphabet.
/*!
 *  \param csa The FM-index (here a compressed suffix array).
 *  \param interval A vector used to output LCP-intervals: {LCP-value, begin, end}.
 *  \praam loc_max A bool to output whether an LCP-interval is a local maximal
 *    (for computing super maximal repeats).
 *
 *  \return The number of left extensions for the output LCP-interval.
 */
template <class csa_wt,
          typename value_type = typename csa_wt::wavelet_tree_type::value_type,
          typename size_type = typename csa_wt::size_type>
Generator<size_type>
lcp_interval_generator(
  const csa_wt& csa, std::vector<size_type>& interval, bool& loc_max)
{

  // initialize text variables
  const size_type n = csa.size();  // text size
  const size_type sigma = csa.wavelet_tree.sigma;  // text alphabet size

  // initialize the outputs
  //bit_vector extensions(sigma, 0);
  std::unordered_set<value_type> extensions;
  interval.resize(3);

  // initialize the finished bit vector
  sdsl::bit_vector finished(n+1, 0);
  finished[0] = finished[n] = 1;

  // initialize interval variables
  size_type l, r;  // left/right boundary of intervals being computed
  size_type lcp_value = 0;  // LCP of interval being computed
  size_type last_idx = 0; // ...
  size_type last_lb = 0;  // ...
  loc_max = true;  // is interval being computed a local maximum

  // initialize a left/right boundary queue for each alphabet letter
  // NOTE: the arrays should be used instead of the vectors when GCC adds
  // support for variable length arrays in coroutines
  //queue<size_type> queues[sigma];
  std::vector<std::queue<size_type>> queues(sigma);
  //size_type queue_sizes[sigma];
  std::vector<size_type> queue_sizes(sigma);
  size_type intervals = 0;
  // store first interval
  for (size_type i = 0; i < sigma; ++i) {
    l = csa.C[i];
    r = csa.C[i+1];
    queues[i].push(l);
    queues[i].push(r);
    intervals += 1;
  }

  // initialize the variables for computing left extensions
  size_type num_symbols;  // the number of unique characters in an extension
  std::vector<value_type> symbols(sigma);  // the unique characters in an extension
  std::vector<size_type> rank_c_rb(sigma);  // the left boundary rank for each character in symbols
  std::vector<size_type> rank_c_lb(sigma);  // the right boundary rank for each character in symbols
  value_type c = 0;

  // initialize loop variables
  size_type i, j, k;

  // loop until all LCP-intervals have been computed
  while (intervals) {
    // set the interval size
    interval[0] = lcp_value;
    // get the queue sizes for the LCP value before adding more intervals
    for (i = 0; i < sigma; ++i) {
      queue_sizes[i] = queues[i].size()/2;
    }
    // iterate the queues in alphabetical order
    for (i = 0; i < sigma; ++i) {
      // iterate all intervals in the queue for the current LCP value
      while (queue_sizes[i]) {
        queue_sizes[i] -= 1;
        // get the interval for this iteration
        size_type lb = queues[i].front();
        queues[i].pop();
        size_type rb = queues[i].front();
        queues[i].pop();
        intervals -= 1;
        if (!finished[rb] || last_idx == lb) {
          // find all left extensions of the interval
          sdsl::interval_symbols(
              csa.wavelet_tree,
              lb, rb,
              num_symbols, symbols,
              rank_c_lb, rank_c_rb
            );
          for (j = 0; j < num_symbols; ++j) {
            // get the symbol
            c = symbols[j];
            // add to the current set of extensions
            extensions.insert(c);
            // skip if it's the delineator
            if (c == 0) {
              continue;
            }
            // get the symbol's alphabet index
            k = csa.char2comp[c];
            // compute the next interval from the symbol
            l = csa.C[k] + rank_c_lb[j];
            r = csa.C[k] + rank_c_rb[j];
            // add the interval to the symbol's queue
            queues[k].push(l);
            queues[k].push(r);
            intervals += 1;
          }
          // add the current LCP-interval
          if (!finished[rb]) {
            finished[rb] = 1;
            // check if lcp-interval wasn't seen before
            if (last_idx != lb) {
              last_lb = lb;
            }
            last_idx = rb;
          // found the last index of the current LCP-interval
          } else if (last_idx == lb) {
            if (lb != rb-1) {
              loc_max = false;
            }
            // output the interval for processing
            interval[1] = last_lb;
            interval[2] = rb-1;
            co_yield extensions.size();
            // reset the interval variables
            extensions.clear();
            last_lb = 0;
            last_idx = 0;
            loc_max = true;
          }
        }
      }
    }
    lcp_value += 1;
  }

}


}

#endif
