
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
#ifndef _BASE_DOUBLEUTIL_H
#define _BASE_DOUBLEUTIL_H

#include <math.h>

namespace jzs {

    inline bool equal(double v1, double v2, double diff = 0.000001)
    {
        return fabs(v1 - v2) < diff;
    }
}

#endif
