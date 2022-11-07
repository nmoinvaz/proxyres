# proxyres - proxy resolution library in C

Cross-platform proxy resolution with support for Linux, macOS, & Windows.

## API

### proxy_config

Read the user's proxy configuration.

### proxy_resolve

Resolves proxies for a given URL based on the operating system's proxy configuration.

* Supports manually specified proxies for specific schemes.
* Supports Web Proxy Auto-Discovery Protocol (WPAD).
* Evaluates any discovered [Proxy Auto-Configuration (PAC)](https://developer.mozilla.org/en-US/docs/Web/HTTP/Proxy_servers_and_tunneling/Proxy_Auto-Configuration_PAC_file) to determine the proxies for the URL.

### proxy_execute

Execute Proxy Auto-Configuration (PAC) files containing the JavasScript function `FindProxyForURL` for a given URL to determine the proxies for it. Support varies depending on the operating system and the libraries available.

|OS|Engine|Info|
|-|-|-|
|Linux|JavaScriptCoreGTK||
|macOS|JavaScriptCore||
|Windows|JSProxy|Deprecated on Windows 11.<br>A copy of jsproxy.dll can be found in the `test` folder.|

## Build

Use CMake to generate project and solution files for your environment:

```bash
cmake -S . -B build
cmake --build build
```

To run CMake tests use the following command:

```bash
ctest --verbose -C Debug
```

## History & Motivation

The code in this library started many years ago as part of a custom HTTP/HTTPS stack we developed at [Solid State Networks](https://solidstatenetworks.com/). When we switched over to using cURL we still needed to implement the proxy resolution. We thought about using libproxy, however the license did not allow us to statically link the library for closed source commerical purposes. With changes that were needed for Windows 11 we decided to move our proxy resolution code to an open-source library since there weren't many alternatives available.

## Todo

* Test integration
* Resolver from scratch with WPAD and DHCP
