#!/bin/sh

mkdir -p java
mkdir -p cpp 
mkdir -p py 

protoc *.proto --cpp_out=cpp --java_out=java --python_out=py
