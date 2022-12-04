#!/bin/bash
rm -rf build
mkdir build
cd build
cmake ..
make clean
make
mkdir -p $DYNLIB_PLUGIN_PATH
echo "Coping dynlib.so to $DYNLIB_PLUGIN_PATH"
cp -v Dynlib.so $DYNLIB_PLUGIN_PATH
cd ..
