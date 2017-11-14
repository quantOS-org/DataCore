
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
#include <atomic>
#include <memory>
#include "shared_ptr_atomic.h"

namespace std _GLIBCXX_VISIBILITY(default)
{

 _GLIBCXX_BEGIN_NAMESPACE_VERSION
#ifdef __GTHREADS
  namespace
  {
    const unsigned char mask = 0xf;
    const unsigned char invalid = mask + 1;

    inline unsigned char key(const void* addr)
    { return _Hash_impl::hash(addr) & mask; }

    /* Returns different instances of __mutex depending on the passed address
     * in order to limit contention.
     */
    __gnu_cxx::__mutex&
    get_mutex(unsigned char i)
    {
      static __gnu_cxx::__mutex m[mask + 1];
      return m[i];
    }
  }

  _Sp_locker::_Sp_locker(const void* p) noexcept
  {
    if (__gthread_active_p())
      {
	_M_key1 = _M_key2 = key(p);
	get_mutex(_M_key1).lock();
      }
    else
      _M_key1 = _M_key2 = invalid;
  }

  _Sp_locker::_Sp_locker(const void* p1, const void* p2) noexcept
  {
    if (__gthread_active_p())
      {
	_M_key1 = key(p1);
	_M_key2 = key(p2);
	if (_M_key2 < _M_key1)
	  get_mutex(_M_key2).lock();
	get_mutex(_M_key1).lock();
	if (_M_key2 > _M_key1)
	  get_mutex(_M_key2).lock();
      }
    else
      _M_key1 = _M_key2 = invalid;
  }

  _Sp_locker::~_Sp_locker()
  {
    if (_M_key1 != invalid)
      {
	get_mutex(_M_key1).unlock();
	if (_M_key2 != _M_key1)
	  get_mutex(_M_key2).unlock();
      }
  }
#endif

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace
