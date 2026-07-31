set auto-load safe-path /usr/lib/
set print thread-events off
set confirm off
set pagination off
set breakpoint pending off

set $dlopen_ptr = (void*(*)(const char*, int)) dlopen
set $dlsym_ptr = (void*(*)(void*, const char*)) dlsym
set $dlerror_ptr = (char*(*)()) dlerror

set $handle = $dlopen_ptr("./build/libgamesneeze.so", 1)
printf "dlopen handle: %p\n", $handle
printf "dlerror: %s\n", $dlerror_ptr()

detach
quit
