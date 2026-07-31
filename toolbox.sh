#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

gdb="gdb"
libname="libgamemodeauto.so.0"
libpath="/tmp/$libname"
libdir="/tmp"
libdeps="/tmp/gamesneeze-deps"
csgo_pid=$(pidof csgo_linux64)

export USE_CLANG="false"

if [[ $EUID -eq 0 ]]; then
    echo "You cannot run this as root." 
    exit 1
fi

rm -rf /tmp/dumps
mkdir -p --mode=000 /tmp/dumps

function unload {
    echo "Unloading cheat..."
    echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
    if grep -q "$libname" "/proc/$csgo_pid/maps"; then
        $gdb -n -q -batch -ex "attach $csgo_pid" \
            -ex "set \$dlopen = (void*(*)(char*, int)) dlopen" \
            -ex "set \$dlclose = (int(*)(void*)) dlclose" \
            -ex "set \$library = \$dlopen(\"/usr/lib/$libname\", 6)" \
            -ex "call \$dlclose(\$library)" \
            -ex "call \$dlclose(\$library)" \
            -ex "detach" \
            -ex "quit"
    fi
    echo "Unloaded!"
}

function load {
    echo "Loading cheat..."
    echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope > /dev/null

    sudo mkdir -p "$libdir" "$libdeps"
    sudo cp "$SCRIPT_DIR/build/libgamesneeze.so" /usr/lib/$libname
    sudo strip --strip-debug /usr/lib/$libname 2>/dev/null || true
    sudo patchelf --set-soname "$libname" /usr/lib/$libname 2>/dev/null || true

    sudo cp /usr/lib/$libname "$libpath"
    sudo chmod 755 "$libpath"

    if [[ -f "$libpath" ]]; then
        sudo patchelf --set-rpath "$libdeps" "$libpath" 2>/dev/null || true
    fi

    $gdb -n -q -batch \
    -ex "set auto-load safe-path /usr/lib/" \
    -ex "set print thread-events off" \
    -ex "set confirm off" \
    -ex "set pagination off" \
    -ex "set breakpoint pending off" \
    -ex "attach $csgo_pid" \
    -ex "set \$dlopen = (void*(*)(const char*, int)) dlopen" \
    -ex "set \$dlsymf = (void*(*)(void*, const char*)) dlsym" \
    -ex "set \$dlerrorf = (char*(*)()) dlerror" \
    -ex "set \$handle = \$dlopen(\"$libpath\", 1)" \
    -ex "printf \"dlopen: %p  dlerror: %s\\n\", \$handle, \$dlerrorf()" \
    -ex "set \$dlerrorf2 = (char*(*)()) dlerror" \
    -ex "set \$init = (void(*)(void))\$dlsymf(\$handle, \"gamesneeze_init\")" \
    -ex "printf \"dlsym: %p  dlerror: %s\\n\", \$init, \$dlerrorf2()" \
    -ex "call \$init()" \
    -ex "detach" \
    -ex "quit"
    
    echo "Successfully injected!"
}

function load_debug {
    echo "Loading cheat..."
    echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
    sudo cp "$SCRIPT_DIR/build/libgamesneeze.so" /usr/lib/$libname
    sudo patchelf --set-soname $libname /usr/lib/$libname
    sudo mkdir -p "$libdir" "$libdeps"
    sudo cp /usr/lib/$libname "$libpath"
    sudo chmod 755 "$libpath"
    $gdb -n -q -batch \
        -ex "set auto-load safe-path /usr/lib:/usr/lib/" \
        -ex "attach $csgo_pid" \
        -ex "set \$dlopen = (void*(*)(char*, int)) dlopen" \
        -ex "call \$dlopen(\"$libpath\", 1)" \
        -ex "detach" \
        -ex "quit"
    $gdb -p "$csgo_pid"
    echo "Successfully loaded!"
}

function build {
    echo "Building cheat..."
    mkdir -p build
    cd build
    cmake -D CMAKE_BUILD_TYPE=Release ..
    make -j $(nproc --all)
    cd ..
}

function build_debug {
    echo "Building cheat..."
    mkdir -p build
    cd build
    cmake -D CMAKE_BUILD_TYPE=Debug ..
    make -j $(nproc --all)
    cd ..
}

function pull {
    git pull
}

while [[ $# -gt 0 ]]
do
keys="$1"
case $keys in
    -u|--unload)
        unload
        shift
        ;;
    -l|--load)
        load
        shift
        ;;
    -ld|--load_debug)
        load_debug
        shift
        ;;
    -b|--build)
        build
        shift
        ;;
    -bd|--build_debug)
        build_debug
        shift
        ;;
    -p|--pull)
        pull
        shift
        ;;
    -h|--help)
        echo "
 help
Toolbox script for gamesneeze the beste lincuck cheat 2021
=======================================================================
| Argument             | Description                                  |
| -------------------- | -------------------------------------------- |
| -u (--unload)        | Unload the cheat from CS:GO if loaded.       |
| -l (--load)          | Load/inject the cheat via gdb.               |
| -ld (--load_debug)   | Load/inject the cheat and debug via gdb.     |
| -b (--build)         | Build to the build/ dir.                     |
| -bd (--build_debug)  | Build to the build/ dir as debug.            |
| -p (--pull)          | Update the cheat.                            |
| -h (--help)          | Show help.                                   |
=======================================================================

All args are executed in the order they are written in, for
example, \"-p -u -b -l\" would update the cheat, then unload, then build it, and
then load it back into csgo.
"
        exit
        ;;
    *)
        echo "Unknown arg $1, use -h for help"
        exit
        ;;
esac
done
