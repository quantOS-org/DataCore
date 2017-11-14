
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

import scala.collection.mutable

/**
  * Created by txu on 2016/9/26.
  */
trait CommonRequestHandler {

    private var _cur_req_id = 1L

    def getNextReqId : Long       = { this._cur_req_id += 1; this._cur_req_id }

    protected val _req_info_map = mutable.LongMap[Any]()

    def saveRequest(req_id: Long, info: Any) {
        _req_info_map += req_id -> info
    }

    def getRequest[T](req_id: Long, default: T, removed: Boolean = true) : T = {

        //logger.error(this.ba_id + " request map: " + _req_info_map)
        val info = _req_info_map.getOrElse(req_id, null)

        //logger.info("getRequest : " + req_id + (if (info==null) " null" else " found"))

        if (info != null && removed )
            _req_info_map -= req_id

        try {
            info.asInstanceOf[T]
        }catch {
            case _: Throwable => default
        }
    }
}
