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

#ifndef INCLUDED_MR_CFG_CFG
#define INCLUDED_MR_CFG_CFG

#include <list>
#include <unordered_map>
#include <utility>  // make_pair, move, pair

#include "mr-cfg/identifier.hpp"
#include "mr-cfg/interval.hpp"
#include "mr-cfg/lcp.hpp"


namespace mr_cfg {


typedef std::list<id_type> CFG_production;
typedef std::unordered_map<id_type, CFG_production> CFG;


//! Builds builds the production for a context-free grammar (CFG) rule.
//
// O(n), excluding CSA-specific operations, where n in the length of the string
// produced by the CFG rule.
/*!
 *  \param csa The compressed suffix array the CFG is being built from.
 *  \param intervals An interval stabbing data-structure containing intervals
 *    of rules already in the grammar.
 *  \param rule_production_sizes A map that associates CFG rule IDs with the
 *    length of the string they produce.
 *  \param cfg The CFG being constructed.
 *  \param i A start position of the rule's string in the input string.
 *  \param n The corresponding end position of the rule's string in the input
 *    string.
 *
 *  \return The computed rule production.
 */
template <class csa_wt,
          typename size_type = typename csa_wt::size_type,
          typename character_type = typename csa_wt::wavelet_tree_type::value_type>
CFG_production computeProduction(
  const csa_wt& csa,
  NestedIntervalStabber<id_type>& intervals,
  std::unordered_map<id_type, size_type>& rule_production_sizes,
  CFG& cfg,
  size_type i,
  const size_type& n)
{
  // create the production
  CFG_production production;
  // iterate the input string from left to right
  while (i < n) {
    // get the suffix array index for i
    size_type j = csa.isa[i];
    // get the longest CFG rule occurrnece that starts at i
    const id_type* rule_id = intervals.stab(j);
    // descend into the rule if it's too long; only necessary if you tinker with
    // making rules longer than reported algorithm
    //while (rule_id != NULL && rule_production_sizes[*rule_id] > n-i) {
    //  rule_id = &(cfg[*rule_id].front());
    //}
    // add a terminal characer if there's no rule
    if (rule_id == NULL) {
      character_type c = csa.text[i];
      size_type c_id = csa.char2comp[c];  // 0 <= c_id < sigma
      production.push_back(c_id);
      // move i to the next character
      i += 1;
    // otherwise, add a non-terminal character
    } else {
      production.push_back(*rule_id);
      // move i to the character after this rule occurrence
      i += rule_production_sizes[*rule_id];
    }
  }
  return production;
}


//! Builds a context-free grammar (CFG) from a compressed suffix array (CSA)
//  implemented with a FM-index and a wavelet tree.
//
// O(n), excluding CSA-specific operations
/*!
 *  \param csa The CSA.
 *
 *  \return The context-free grammar.
 */
template <class csa_wt, typename size_type = typename csa_wt::size_type>
std::pair<CFG, id_type> csaToCfg(const csa_wt& csa, const std::string& algorithm) {

  size_type sigma = csa.wavelet_tree.sigma;

  // initialize the output CFG and the supporting size map
  CFG cfg;
  std::unordered_map<id_type, size_type> rule_production_sizes;
  for (id_type i = 0; i < sigma; ++i) {
    rule_production_sizes[i] = 1;
  }

  // initialize the interval stabbing data-structure
  NestedIntervalStabber<id_type>* intervals;
  if (algorithm == "OPTIMAL") {
    intervals = new OptimalNestedIntervalStabber<id_type, csa_wt>(csa);
  } else if (algorithm == "ONLINE") {
    intervals = new OnlineNestedIntervalStabber<id_type>;
  } else {  // "FAST"
    intervals = new FastNestedIntervalStabber<id_type>;
  }

  // prepare to compute LCP-intervals
  std::vector<size_type> interval;  // {LCP-value, begin, end}
  bool loc_max;
  auto lcp_intervals = lcp_interval_generator(csa, interval, loc_max);

  // initialize a position-to-ID map
  OnlineLcpIdentifiers repeat_ids(csa);

  // skip the length 0 LCP-interval
  lcp_intervals.next();

  // compute LCP-intervals
  for (auto left_extensions: lcp_intervals) {
    // compute the repeat's ID
    id_type repeat_id = repeat_ids.getId(interval[0], interval[1], interval[2]);
    // create a rule in the CFG for the ID if necessary
    if (!rule_production_sizes.contains(repeat_id)) {
      // actually don't need to create the rule; just the the size
      rule_production_sizes[repeat_id] = 0;
    }
    rule_production_sizes[repeat_id] += 1;
    // check if the interval is maximal
    if (*left_extensions > 1) {
      // construct the rule
      size_type i = csa[interval[1]];
      const size_type n = i + rule_production_sizes[repeat_id];
      cfg[repeat_id] =
        computeProduction(csa, *intervals, rule_production_sizes, cfg, i, n);
      // add the rule's repeat to the interval stabber if it's large enough
      if (cfg[repeat_id].size() > 1) {
        //intervals.update(interval[1], interval[2], repeat_id);
        intervals->update(interval[1], interval[2], repeat_id);
      // otherwise, remove the rule from the CFG
      } else {
        cfg.erase(repeat_id);
        rule_production_sizes.erase(repeat_id);
      }
      // erase the ID to guarantee left-extensions will use a different ID
      repeat_ids.removeId(interval[0], interval[1], interval[2]);
    }
  }

  // compute the start rule
  id_type start_rule = repeat_ids.getNextId();
  size_type i = 0;
  const size_type n = csa.size();
  cfg[start_rule] =
    computeProduction(csa, *intervals, rule_production_sizes, cfg, i, n);

  return std::make_pair(std::move(cfg), start_rule);

}


//! Prints a context-free grammar (CFG) to the standard error.
/*!
 *  \param csa The compressed suffix array the grammar was built from.
 *  \param cfg The grammar.
 *  \param start_rule The start rule of the grammar.
 *
 *  \return void.
 */
template <class csa_wt>
void printCfg(const csa_wt& csa, CFG& cfg, id_type start_rule) {
  // naive approach that just traverses grammar
  if (start_rule < csa.sigma) {
    // skip the terminating character
    if (start_rule > 0) {
      std::cerr << csa.comp2char[start_rule];
    }
    return;
  }
  for (const id_type& rule: cfg[start_rule]) {
    printCfg(csa, cfg, rule);
  }
}


}

#endif
