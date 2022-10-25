# proxyres - proxy resolution library in C

Cross-platform support for linux, macOS, & Windows.

**TODO**
* Threaded async for WinXP
* Support for macOS
* Unit tests with Google Test
* Test integration
* PAC execution (WebKitJS & WinInet Pre-Win11)
* Cross-platform Proxy Configuration reading/writing
* Resolver from scratch with WPAD and DHCP

## Build

Use CMake to generate project and solution files for your environment:

```
cmake -S . -B build
cmake --build build
```
