rm -rf build
mkdir build
chmod 755 build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. -DDOPARSE=TRUE
make VERBOSE=1

