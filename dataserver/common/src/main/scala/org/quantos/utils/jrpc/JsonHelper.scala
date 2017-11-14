
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

package org.quantos.utils.jrpc

import java.lang.reflect.{ParameterizedType, Type}

import com.fasterxml.jackson.annotation.JsonInclude.Include
import com.fasterxml.jackson.core.`type`.TypeReference
import com.fasterxml.jackson.databind.{JsonNode, ObjectMapper}
import com.fasterxml.jackson.module.scala.DefaultScalaModule

object JsonHelper {

    val mapper = new ObjectMapper()

    mapper.setSerializationInclusion(Include.NON_NULL)
    mapper.registerModule(DefaultScalaModule)

    def serialize(value: Any): String = mapper.writeValueAsString(value)

    def deserialize[T: Manifest](value: String) : T = mapper.readValue(value, typeReference[T])

    def toJson(value: String): JsonNode = mapper.readTree(value)

    private [this] def typeReference[T: Manifest] = new TypeReference[T] {
        override def getType = typeFromManifest(manifest[T])
    }

    private[this] def typeFromManifest(m: Manifest[_]): Type = {
        if (m.typeArguments.isEmpty) {
            m.erasure 
        } else{
            new ParameterizedType {
                def getRawType = m.erasure
                def getActualTypeArguments = m.typeArguments.map(typeFromManifest).toArray
                def getOwnerType = null
            }
        }
    }
    
    // WARNING: very slow when big object
    def convert[T: Manifest] (obj: Any): T = {
        mapper.convertValue(obj, typeReference[T])
    }

    def convert[T: Manifest] (obj: Any, m: Manifest[T]): T = {
        mapper.convertValue(obj, new TypeReference[T] {
            override def getType = typeFromManifest(m)
        })
    }

}
