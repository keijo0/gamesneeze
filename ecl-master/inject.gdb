set auto-load safe-path /usr/lib/
set print thread-events off
set confirm off
set pagination off
set breakpoint pending off

set $dlopen_ptr = (void*(*)(const char*, int)) dlopen
set $dlsym_ptr = (void*(*)(void*, const char*)) dlsym
set $dlerror_ptr = (char*(*)()) dlerror

set $handle = $dlopen_ptr("/tmp/libMangoHud.so", 1)
printf "dlopen handle: %p\n", $handle
printf "dlerror: %s\n", $dlerror_ptr()

set $init_func = (void(*)()) $dlsym_ptr($handle, "eclipse_init")
printf "eclipse_init: %p\n", $init_func
printf "dlerror after dlsym: %s\n", $dlerror_ptr()

call $init_func()

detach
quit
