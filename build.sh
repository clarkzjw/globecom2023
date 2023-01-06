make clean
rm -rf CMakeFiles
rm -f CMakeCache.txt
cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_FLAGS_DEBUG="-g -O0" -DCMAKE_CXX_FLAGS_DEBUG="-g -O0" .
make -j`nproc --all`
rm -rf ./tmp/*.svc
rm -rf ./tmp/*.mpd
rm -rf ./tmp/*.m4s
rm -rf ./qlog/*.qlog
rm -rf ./qlog/*.log
