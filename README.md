# proxyres - proxy resolution library in C

Cross-platform proxy resolution with support for Linux, macOS, & Windows.

## Features

* Read user's proxy configuration from `IE`, `CFNetwork`, `GSettings`, `KDE user config`, or `Environment Variables`.
* Execute a PAC file using `Windows Script Host`, `JSProxy`, or `JavaScriptCore`.
* Get the proxies for a URL asynchronously using `WinHTTP`, `CFNetwork`, `Gnome3`, or a custom fallback resolver.
* POSIX compatible fallback proxy resolver with support for `WPAD` using `DHCP` and `DNS`.
  * Starts `WPAD` discovery upon intiailization, caches every 5 minutes, and refreshes.
* Documentation and example command line utility.

## API

|Class|Description|
|-|:-|
|[proxy_config](./doc/proxy_config.md)|Read the user's proxy configuration.|
|[proxy_execute](./doc/proxy_execute.md)|Executes a Proxy Auto-Configuration (PAC) script containing the JavasScript function `FindProxyForURL` for a particular URL to determine its proxies.|
|[proxy_resolver](./doc/proxy_resolver.md)|Resolves proxies for a given URL based on the operating system's proxy configuration.|

## Example

Source code examples can be found in the docs for each class or in the command line test application [proxycli](./test/proxycli.c).

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

Portions of the code for this library started many years ago as part of a custom HTTP/HTTPS stack we developed at [Solid State Networks](https://solidstatenetworks.com/). Recently we made the decision to switch over to `libcurl` but realized that we still needed to implement our own proxy resolution code. We investigated using `libproxy`, however it did not allow us to statically link the library for closed source commerical purposes.

## Supported Platforms

* Windows XP+
* Windows RT (UWP)
* Ubuntu
* macOS

## Resources

* [Wikipedia - Proxy Auto-config](https://en.wikipedia.org/wiki/Proxy_auto-config)
* [Mozilla - Proxy Auto-Configuration (PAC)](https://developer.mozilla.org/en-US/docs/Web/HTTP/Proxy_servers_and_tunneling/Proxy_Auto-Configuration_PAC_file)
* RFC Drafts
  * [Web Proxy Auto-Discovery Protocol](https://datatracker.ietf.org/doc/html/draft-ietf-wrec-wpad-01)
  * [Generalize Client UDP Port Number of DHCP Relay](https://datatracker.ietf.org/doc/html/draft-shen-dhc-client-port-00)
