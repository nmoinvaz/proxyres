# proxyres - proxy resolution library in C

Cross-platform proxy resolution with support for Linux, macOS, & Windows.

## API

### proxy_config

Read the user's proxy configuration.

### proxy_resolve

Resolves proxies for a given URL based on the operating system's proxy configuration.

* Supports manually specified proxies for specific schemes.
* Supports Web Proxy Auto-Discovery Protocol (WPAD) using DHCP and DNS.
* Evaluates any discovered [Proxy Auto-Configuration (PAC)](https://developer.mozilla.org/en-US/docs/Web/HTTP/Proxy_servers_and_tunneling/Proxy_Auto-Configuration_PAC_file) scripts to determine the proxies for the URL.

**Operating System Support**

The following native OS proxy resolution libraries are used if available:

|OS|Library|Info|
|-|-|-|
|Linux|Gnome3/GProxyResolver|Does not return anything other than direct:// on Ubuntu 20.|
|macOS|CFNetwork||
|Windows|WinHTTP|Uses IE proxy configuration.|

When there is no built-in proxy resolution library on the system, we use our own resolver.

### proxy_execute

Executes a Proxy Auto-Configuration (PAC) script containing the JavasScript function `FindProxyForURL` for a particular URL to determine the proxies for it. Support varies depending on the operating system and the libraries available.

#### Script Engine Support

|OS|Engine|Info|
|-|-|-|
|Linux|JavaScriptCoreGTK|Dynamically loaded at run-time.|
|macOS|JavaScriptCore|Dynamically loaded at run-time.|
|Windows|JSProxy|Deprecated on Windows 11.<br>A copy of jsproxy.dll can be found in the `test` folder.|
|Windows|Windows Script Host|Uses IActiveScript COM interfaces.|

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

* Enumerate each adapter and check each DHCP for better security
* Cache results from WPAD process for certain period of time
* Mutex lock around WPAD process
* Clean up proxy_resolver API
* Test integration
* Document API
