export LD_LIBRARY_PATH=../lib
g++ -o demo main.cpp -I../include -L../lib -lTDFAPI30 -lrt
./demo
