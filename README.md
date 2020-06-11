# Warlock
This project was done purely for educational purposes. The driver provides a process with indirect read/write access to another process's memory from ring 0. An example use case is if access to a process is being defended from kernel mode.



## Usage

1. Create custom client by adding to [Client.cpp](https://github.com/Tserith/Warlock/blob/master/Client/Client.cpp)
2. Optionally change client process [name](https://github.com/Tserith/Warlock/blob/master/Warlock/Common.h#L5)
3. Load driver and run client program



## Features

- Can access user memory regardless of page protection (Dirties executable pages on write)
- Does not create any kernel objects (excluding it's own DRIVER_OBJECT)
- Does not create any system threads



## Limitations

- Only supports 64-bit processes
- Only supports one client and one target process at a time
- Only accesses 8 bytes per request