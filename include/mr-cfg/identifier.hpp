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

#ifndef INCLUDED_MR_CFG_IDENTIFIER
#define INCLUDED_MR_CFG_IDENTIFIER

#include <unordered_map>


namespace mr_cfg {


typedef uint64_t id_type;


//! A class that assigns IDs to LCP-intervals based on end positions.
template <class csa_wt,
          typename value_type = typename csa_wt::value_type,
          typename size_type = typename csa_wt::size_type>
class OnlineLcpIdentifiers
{

private:

  const csa_wt& _csa;
  id_type _id;
  std::unordered_map<size_type, id_type> _repeat_ids;

public:

  OnlineLcpIdentifiers(const csa_wt& csa): _csa(csa) {
    // the first \sigma IDs are reserved for the alphabet characters
    _id = csa.sigma;
  }

  //! Gets the ID that will be assigned to the next LCP-interval that doesn't
  //  have an ID.
  id_type getNextId() const {
    return _id;
  }

  //! Gets the ID for the given LCP-interval. IDs are assigned based on the end
  //  position of the first CSA entry in the interval.
  id_type
  getId(const size_type& value, const size_type& begin, const size_type& end)
  {
    size_type first_position = _csa[begin] + value;
    if (!_repeat_ids.contains(first_position)) {
      _repeat_ids[first_position] = _id;
      _id += 1;
    }
    return _repeat_ids[first_position];
  }

  //! Removes the ID for the given LCP-interval. Actually removes the position
  //  the interval's ID is based on from the position-ID map.
  void
  removeId(const size_type& value, const size_type& begin, const size_type& end)
  {
    size_type first_position = _csa[begin] + value;
    if (_repeat_ids.contains(first_position)) {
      _repeat_ids.erase(first_position);
    }
  }

};


}

#endif
