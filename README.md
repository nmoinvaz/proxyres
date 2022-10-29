# proxyres - proxy resolution library in C

Cross-platform support for Linux, macOS, & Windows.

## Interfaces

**proxy_config**

Read and write the operating system's proxy configuration information.

**proxy_resolve**

Resolves proxies for a given URL based on the operating system's proxy configuration.

* Supports manually specified proxies for specific protocols.
* Supports Web Proxy Auto-Discovery Protocol (WPAD).
* Evaluates any discovered Proxy Auto-Configuration (PAC) to determine the proxies for the URL.

**proxy_execute**

Execute Proxy Auto-Configuration (PAC) files containing the JavasScript function `FindProxyForURL` for a given URL to determine the proxies for it. Support varies depending on the operating system and the libraries available.

|OS|Engine|Info|
|-|-|-|
|Linux|JavaScriptCoreGTK||
|macOS|JavaScriptCore||
|Windows|JSProxy|Deprecated on Windows 11.<br>A copy of jsproxy.dll can be found in the `test` folder.|

## Build

Use CMake to generate project and solution files for your environment:

```
cmake -S . -B build
cmake --build build
```

## Todo

* Multi-threaded proxy resolver
* Support for macOS
* Unit tests with Google Test
* Test integration
* Cross-platform Proxy Configuration reading/writing
* Resolver from scratch with WPAD and DHCP
