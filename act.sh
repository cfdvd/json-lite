clear

/usr/bin/cmake -B "build" -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON

cmake --build "build" --target "all" --config "Debug" --parallel 

echo ""
cd ./build
chmod +x test
clear
./test 
