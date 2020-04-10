# Warlock
This project was done purely for educational purposes. I am aware that this tool could be used to evade anti-cheat in games. If you use this to cheat and it gets signatured that's not my problem.



## Usage

1. Define [client process name](https://github.com/Tserith/Warlock/blob/master/Warlock/Common.h#L5)
2. Create custom client by adding to [Client.cpp](https://github.com/Tserith/Warlock/blob/master/Client/Client.cpp)
3. Load driver and run client program



## Features

- Can access user memory regardless of page protection
- Driver does not create any kernel objects (excluding it's own DRIVER_OBJECT)
- Driver does not create any system threads



## Limitations

- Only supports 64-bit processes
- Only supports one client and one target process at a time
- Only accesses 8 bytes per request