
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

import java.io.StringWriter
import java.lang.reflect.{ParameterizedType, Type}

import com.fasterxml.jackson.annotation.JsonInclude.Include
import com.fasterxml.jackson.core.`type`.TypeReference
import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.dataformat.csv.CsvMapper
import com.fasterxml.jackson.module.scala.DefaultScalaModule

/**
  * Created by txu on 2017/8/24.
  */
object CsvHelper {

    val mapper = new CsvMapper()
    mapper.setSerializationInclusion(Include.NON_NULL)
    mapper.registerModule(DefaultScalaModule)
    mapper.configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false)


    def serialize[T:Manifest](values: Seq[T]): String = {
        val writer = new StringWriter()

        val schema = mapper.schemaFor(typeReference[T]).withHeader()
        val csv_writer = mapper.writerFor(typeReference[T]).`with`(schema).writeValues(writer)

        values foreach { csv_writer.write(_) }

        writer.toString
    }

    def deserialize[T: Manifest](value: String) : Seq[T] = {
        val schema = mapper.schemaFor(typeReference[T]).withHeader()
        val it = mapper.readerFor(typeReference[T]).`with`(schema).readValues[T](value)

        it.readAll().toArray.asInstanceOf[Array[T]]
    }

    //    def toCSV(value: String): JsonNode = mapper.readTree(value)

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


    def convert[T: Manifest] (obj: Any): T = {
        mapper.convertValue(obj, typeReference[T])
    }

}
