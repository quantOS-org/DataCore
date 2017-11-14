
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

package org.quantos.jads.utils

/**
  * Created by hfan on 2016/12/14.
  */
object MathUtil {
    
    val LONG_NAN = 9223372036854775807L

    def min(v1: Long, v2: Long) = if(v1<v2) v1 else v2

    def min(v1: Long, v2: Long, v3: Long) = if(v1<v2) { if(v1<v3) v1 else v3 } else { if(v2<v3) v2 else v3 }

    def min(v1: Long, v2: Long, v3: Long, v4: Long) = if(v1<v2) { if(v1<v3) {if(v1<v4) v1 else v4} else {if(v3<v4) v3 else v4 }} else { if(v2<v3) {if(v2<v4) v2 else v4} else {if(v3<v4) v3 else v4} }

    def round(num: Double): Double = if (num==0) { 0 } else if (num>0) { (num + 0.5).toInt * 1d } else { (num - 0.5).toInt * 1d }

    def round2(num: Double): Double = if (num==0) { 0 } else if (num>0) { (num * 100 + 0.5).toInt / 100d } else { (num * 100 - 0.5).toInt / 100d } /* 四舍五入到两位小数 */

    def round4(num: Double): Double = if (num==0) { 0 } else if (num>0) { (num * 10000 + 0.5).toInt / 10000d } else { (num * 100 - 0.5).toInt / 100d }/* 四舍五入到四位小数 */

    def double_equals(v1: Double, v2: Double): Boolean = {
        val v = v1 - v2
        v > -0.0000001 && v < 0.0000001
    }

    def double_in_range(v:Double, r1: Double, r2: Double): Boolean = {
        v+0.0000001 > r1 && v-0.0000001 < r2
    }
}
