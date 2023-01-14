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

#ifndef INCLUDED_MR_CFG_INTERVAL
#define INCLUDED_MR_CFG_INTERVAL

#include <limits>
#include <map>
#include <stack>
#include <unordered_map>
#include <vector>

#include <roaring/roaring64map.hh>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/select_support_mcl.hpp>

#include "mr-cfg/interval.hpp"
#include "mr-cfg/lcp.hpp"


namespace mr_cfg {


//! An abstract class that defines the interface of our novel data-structure for
//  answering stabbing queries on nested intervals over a finite range [0..n].
template <typename element_type>
class NestedIntervalStabber
{

public:

  //! Performs a stabbing query on the intervals and returns the deepest nested
  //  updated interval stabbed.
  //
  //  Note: Generally all stabbed intervals can be returned by maintaining a
  //  nested interval tree internally, but here we only need the deepest nested
  //  interval so no tree is necessary.
  /*!
   *  \param i The point to stab.
   *
   *  \return A pointer to the ID of the interval stabbed, if any. Otherwise NULL.
   */
  virtual const element_type* stab(const uint64_t& i) = 0;

  //! Adds an interval so it can be returned by a stabbing query.
  /*!
   *  \param begin The begin postion of the interval to update.
   *  \param end The end position of the interval to update.
   *  \param id The ID of the interval to update.
   */
  virtual void
  update(const uint64_t& begin, const uint64_t& end, const element_type& id) = 0;

};


//! An implementation of our novel interval stabbing data-structure that uses a
//  sorted map and binary search. This is the "online" algorithm described in
//  the paper.
template <typename element_type>
class OnlineNestedIntervalStabber: public NestedIntervalStabber<element_type>
{

private:

  // maps selected bits to interval IDs
  std::map<uint64_t, element_type> _lookup;

public:

  const element_type* stab(const uint64_t& i) {
    // return NULL if the map is empty
    if (_lookup.empty()) {
      return NULL;
    }
    // get the first element with a key that is not less than i
    auto iter = _lookup.lower_bound(i);
    // return the element before i if i is not in the map
    if (iter == _lookup.end() || iter->first != i) {
      // return NULL if there is no element before i
      if (iter == _lookup.begin()) {
        return NULL;
      } else {
        --iter;
      }
    }
    // return NULL if the element is the placeholder
    if (iter->second == std::numeric_limits<element_type>::max()) {
      return NULL;
    }
    return &(iter->second);
  }

  //! Adds an interval assuming it's nested in an existing interval if there's any overlap.
  void update(const uint64_t& begin, const uint64_t& end, const element_type& id) {
    // get the ID of the interval this interval will be nested in
    const element_type* parent_id = stab(begin);
    // if the end bit is already set then:
    // 1) it's already set by another end and we don't want to update the lookup; or
    // 2) it's already set by a begin and we don't want to change it
    // map.try_emplace will only add an element if the key doesn't exist so it
    // automatically handles these scenarios
    if (parent_id == NULL) {
      _lookup.try_emplace(end+1, std::numeric_limits<element_type>::max());
    } else {
      _lookup.try_emplace(end+1, *parent_id);
    }
    // add the beginning of the interval
    _lookup[begin] = id;
  }

};


//! An implementation of our novel interval stabbing data-structure that
//  preprocesses LCP-intervals for O(1) time stabbing queries. This is the
//  "optimal" algorithm described in the paper.
//
//  Since all LCP-intervals must be indexed during preprocessing, updates are
//  supported by assigning internal IDs to the intervals such that the IDs
//  reflect the nested structure of the intervals. Specifically, each ID is a
//  bit vector where each bit corresponds to a specific interval. Each
//  interval's ID has its bit set as well as the bits of the intervals it's
//  nested in. This allows the nearest updated ancestor of an interval to be
//  computed efficiently using bitwise operations.
//
//  This isn't truly optimal since the number of words required to represent
//  each ID (and therefore the number of CPU instructions required per operation)
//  is O(m/w), where m is the number of maximal repeats and w is the size of a
//  word on the processor. Even with bit-level parallelism, the number of CPU
//  cycles required is a function of m and w. However, the number of maximal
//  repeats in real data tends to be relatively small, and assigning bits to
//  repeats in depth-first order tends to create runs of bits in the IDs. Here
//  we use compressed bit vectors that utilize bit-level operations supported by
//  the CPU, allowing large, sparse IDs to be stored in small space with near
//  optimal run-time performance.
template <typename element_type,
          class csa_wt,
          typename value_type = typename csa_wt::wavelet_tree_type::value_type,
          typename size_type = typename csa_wt::size_type>
class OptimalNestedIntervalStabber: public NestedIntervalStabber<element_type>
{

private:

  // maps selected bits to binary IDs
  std::unordered_map<size_type, roaring::Roaring64Map*> _lookup;
  // stores the begin and end+1 positions of intervals
  sdsl::bit_vector _position_bits;
  // supports O(1) time rank queries on _position bits
  sdsl::rank_support_v5<> _rank;
  // supports O(1) time select queries on _position_bits
  sdsl::select_support_mcl<> _select;
  // the ID that tracks what intervals have been updated
  roaring::Roaring64Map* _update_id;
  // an array to store repeat IDs
  roaring::Roaring64Map* *_ids;
  // maps binary IDs to external IDs
  std::unordered_map<uint32_t, element_type> _id_map;

  //! Initializes data-structures by computing LCP-intervals for the given
  //  compressed suffix array (CSA) then iterating them in begin-end order. An
  //  ID is assigned to each maximal repeat LCP-interval that reflects what
  //  other intervals it's nested in. Note that computing these IDs internally
  //  and mapping them to external IDs is not space optimal. We use this
  //  approach here so that the same IDs can be used externally accross
  //  NestedIntervalStabber implementations.
  //
  //  O(n) time, where n is the size of the CSA.
  /*!
   *  \param csa The compressed suffix array to compute LCP-intervals for.
   */
  void initialize(const csa_wt& csa) {

    const size_type n = csa.size();

    // initialize the bit vector
    _position_bits = sdsl::bit_vector(n, 0);

    // prepare to compute LCP-intervals
    std::vector<size_type> interval;  // {LCP-value, begin, end}
    bool loc_max;
    auto lcp_intervals = lcp_interval_generator(csa, interval, loc_max);

    // skip the length 0 LCP-interval
    lcp_intervals.next();

    // compute LCP-intervals while counting maximal repeats and binning their
    // end positions by begin position
    size_type num_repeats = 0;
    size_type num_bits = 0;
    std::unordered_map<size_type, std::list<size_type>> repeat_bins;
    for (auto left_extensions: lcp_intervals) {
      // check if the interval is maximal
      if (*left_extensions > 1) {
        // count the repeat
        num_repeats += 1;
        // set the begin bit
        if (_position_bits[interval[1]] == 0) {
          _position_bits[interval[1]] = 1;
          num_bits += 1;
        }
        // set the end bit
        size_type end = interval[2]+1;
        if (end < n && _position_bits[end] == 0) {
          _position_bits[end] = 1;
          num_bits += 1;
        }
        // bin the interval
        repeat_bins[interval[1]].push_back(interval[2]);
      }
    }

    // initialize the update ID and prepare to compute repeat IDs
    _update_id = new roaring::Roaring64Map();
    _ids = new roaring::Roaring64Map*[num_repeats];
    num_repeats -= 1;
    _lookup.reserve(num_bits);

    // dovetail iterate begin and end positions in order
    std::stack<size_type> end_stack;
    std::stack<roaring::Roaring64Map*> id_stack;
    id_stack.push(_update_id);
    // -1 because intervals don't start at the last position and end+1 is out
    // of bounds at the last position
    for (size_type i = 0; i < _position_bits.size()-1; ++i) {
      // pop all end positions equal to i
      while (!end_stack.empty()) {
        if (end_stack.top() == i) {
          end_stack.pop();
          // add the parent ID to _lookup
          id_stack.pop();
          if (id_stack.size() > 1) {
            _lookup[i+1] = id_stack.top();
          }
        } else {
          break;
        }
      }
      // generate an ID for each interval that starts at i
      if (repeat_bins.contains(i)) {
        // iterate the binned end positions
        for (const size_type& end: repeat_bins[i]) {
          // add the end position to the stack
          end_stack.push(end);
          // compute an ID for the interval derived from the parent ID
          _ids[num_repeats] = new roaring::Roaring64Map(*id_stack.top());
          _ids[num_repeats]->add(num_repeats);
          id_stack.push(_ids[num_repeats]);
          num_repeats -= 1;
        }
        // add the last computed (deepest) ID to _lookup
        roaring::Roaring64Map* top = id_stack.top();
        size_type max_size = _lookup.max_size();
        _lookup[i] = top;
      }
    }

    // initialize rank structure
    _rank = sdsl::rank_support_v5<>(&_position_bits);

    // initialize select structure
    _select = sdsl::select_support_mcl(&_position_bits);

  }

  //! Performs a stabbing query on the intervals and returns the binary ID of
  //  the deepest nested interval stabbed.
  const roaring::Roaring64Map* _stab(const uint64_t& i) const {
    // get the rank, i.e. how many bits are set up to i
    // +1 because SDSL rank is exclusive
    size_type rank = _rank.rank(i+1);
    // rank 0 means there's no element to get
    if (rank == 0) {
      return NULL;
    }
    // get the position of the rankth bit
    size_type j = _select.select(rank);
    // lookup the deepest nested interval that set the bit
    if (_lookup.contains(j)) {
      return _lookup.at(j);
    }
    return NULL;
  }

public:

  OptimalNestedIntervalStabber(const csa_wt& csa) {
    initialize(csa);
  }

  ~OptimalNestedIntervalStabber() {
    // deallocate the update pointer
    _update_id = NULL;
    delete _update_id;
    // deallocate the ID array
    delete [] _ids;
  }

  //! Performs a stabbing query on the intervals and returns the deepest nested
  //  interval stabbed that has been updated.
  const element_type* stab(const uint64_t& i) {
    // get the binary ID of the deepest nested interval
    const roaring::Roaring64Map* binary_id = _stab(i);
    // NULL means there's no element to get
    if (binary_id == NULL) {
      return NULL;
    }
    // compute the deepest ancestor that has been updated
    const roaring::Roaring64Map ancestor_id = *_update_id & *binary_id;
    // return the external ID if the ancestor exists
    const uint64_t interval_bit = ancestor_id.minimum();
    if (_id_map.contains(interval_bit)) {
      return &_id_map.at(interval_bit);
    }
    return NULL;
  }

  //! Assigns an ID to an interval already in the structure so it can be
  //  returned by stabbing queries.
  void update(const uint64_t& begin, const uint64_t& end, const element_type& id) {
    // get the binary IDs of the deepest intervals the begin and end positions stab
    const roaring::Roaring64Map* begin_id = _stab(begin);
    const roaring::Roaring64Map* end_id = _stab(end);
    // compute the lowest common ancestor ID, i.e. the ID originally assigned to
    // the interval
    const roaring::Roaring64Map interval_id = *begin_id & *end_id;
    // save the ID mapping
    //uint32_t interval_bit = interval_id.minimum();
    const uint64_t interval_bit = interval_id.minimum();
    _id_map[interval_bit] = id;
    // make the ID discoverable by stabbing queries
    *_update_id |= interval_id;
  }

};


//! An implementation of our novel interval stabbing data-structure that uses a
//  dynamic bitmap.
template <typename element_type>
class FastNestedIntervalStabber: public NestedIntervalStabber<element_type>
{

private:

  // maps selected bits to interval IDs
  std::unordered_map<uint64_t, element_type> _lookup;
  // stores the begin and end+1 positions of intervals
  roaring::Roaring64Map _position_bits;

public:

  const element_type* stab(const uint64_t& i) {
    // get the rank, i.e. how many bits are set up to i
    uint64_t rank = _position_bits.rank(i);
    // rank 0 means there's no element to get
    if (rank == 0) {
      return NULL;
    }
    // get the position of the rankth bit
    uint64_t j;
    // -1 because roaring select is 0 based
    bool has_bit = _position_bits.select(rank-1, &j);
    // lookup the deepest nested interval that set the bit
    if (has_bit && _lookup.contains(j)) {
      return &_lookup[j];
    }
    return NULL;
  }

  //! Adds an interval assuming it's nested in an existing interval if there's any overlap.
  void update(const uint64_t& begin, const uint64_t& end, const element_type& id) {
    // get the ID of the interval this interval will be nested in
    const element_type* parent_id = stab(begin);
    // if the end bit is already set then:
    // 1) it's already set by another end and we don't want to update the lookup; or
    // 2) it's already set by a begin and we don't want to change it
    if (!_position_bits.contains(end+1)) {
      _position_bits.add(end+1);
      // save space by only saving IDs that aren't NULL
      if (parent_id != NULL) {
        _lookup[end+1] = *parent_id;
      }
    }
    // add the beginning of the interval here so we don't change parent_id's value before
    // dereferencing
    _position_bits.add(begin);
    _lookup[begin] = id;
  }

};


}

#endif
