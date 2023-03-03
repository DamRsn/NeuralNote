## Clone with submodule:
Use this when cloning:
```
git clone --recurse-submodules ...
 ```

or do this after a normal clone
``` 
git submodule init
git submodule update
```

## IDEs ...
To build, all you have to do is load this project in your favorite IDE 
(CLion/Visual Studio/VSCode/etc) 
and click 'build' for one of the targets.

You can also generate a project for an IDE by using (Mac):
```
cmake -G Xcode -B build
```
Windows:
```
cmake -G "Visual Studio 16 2022" -B build
```

## Build with CMake from terminal
```
mkdir build
cd build
cmake -DBUILD_UNIT_TESTS=ON ..
cmake --build .
```