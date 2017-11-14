
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
#ifndef JZS_MSGQUEUE_H
#define JZS_MSGQUEUE_H  

#include <thread>
#include <condition_variable>
#include <mutex>
#include <list>

using namespace std;

namespace jzs {


  template <typename Elem>
  class MsgQueue {
  public:
  public:    
    MsgQueue()
    {
    }

    virtual ~MsgQueue() {
    }

    //uint64_t Size() {
    //  
    //}

    bool Push(const Elem& data) {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_queue.push_back(data);
        m_cond.notify_all();
        return true;
    }

    bool Pop(Elem& data, int ms) {
        std::unique_lock<std::mutex> lck(m_mtx);
        do {
            if (m_queue.size()) {
                data = m_queue.front();
                m_queue.erase(m_queue.begin());
                return true;
            } else {
                if (ms <= 0) return false;
                if (m_cond.wait_for(lck, chrono::milliseconds(ms)) == std::cv_status::timeout)
                    return false;
                ms = 0;
            }
        } while (true);
        return false;
    }


  private:
    std::list<Elem>         m_queue;
    std::condition_variable m_cond;
    std::mutex              m_mtx;
  };
  
}

#endif 
