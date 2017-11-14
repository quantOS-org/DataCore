
md java
md cpp 
md py 

protoc jzs.proto --cpp_out=cpp --java_out=java --python_out=py
protoc md.proto  --cpp_out=cpp --java_out=java --python_out=py
protoc qms.proto --cpp_out=cpp --java_out=java --python_out=py
protoc misc.proto --cpp_out=cpp --java_out=java --python_out=py

pause