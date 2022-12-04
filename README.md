# sc3dynlib

## Install (Linux)

Set paths:

SuperCollider SC_PATH and SuperCollider Extensions path

```
export SC_PATH=/usr/include/SuperCollider/
export DYNLIB_CLASS_PATH=$HOME'/.local/shareSuperCollider/Extensions/Dynlib/classes/'
export DYNLIB_PLUGIN_PATH=$HOME'/.local/share/SuperCollider/Extensions/Dynlib/plugins/'
```

Building

```
cd src
./build_and_cp.sh
```

## Build Plugin (Linux)

```
g++ -fpic -c DemoSin.cpp -o DemoSin.o -Ofast
gcc -fpic -shared DemoSin.o -lm -o libDemoSin.so -Ofast
```