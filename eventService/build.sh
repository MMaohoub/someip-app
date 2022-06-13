cd client
rm -rd ./build
mkdir build
cd build
cmake ..
make
cd ../server
rm -rd ./build
mkdir build
cd build
cmake ..
make
./client/build/client
./server/build/server

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./client/build/vsomeip/:./server/build/vsomeip/