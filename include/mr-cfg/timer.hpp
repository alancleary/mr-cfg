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

#ifndef INCLUDED_MR_CFG_TIMER
#define INCLUDED_MR_CFG_TIMER

#include <chrono>


namespace mr_cfg {


//! Encapsulates simple code for measuring run-time.
class Timer
{

private:

  std::chrono::high_resolution_clock::time_point _start_time;
  std::chrono::high_resolution_clock::time_point _task_start_time;

public:

  Timer() {
    _start_time = std::chrono::high_resolution_clock::now();
  }

  void startTask() {
    _task_start_time = std::chrono::high_resolution_clock::now();
  }

  void endTask() {
    std::chrono::high_resolution_clock::time_point end_time =
      std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - _task_start_time);
    std::cout << "task: " << duration_ms.count() << "ms" << std::endl;
    duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - _start_time);
    std::cout << "total: " << duration_ms.count() << "ms" << std::endl;
  }

};


}

#endif
