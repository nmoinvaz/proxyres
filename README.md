# proxyres - proxy resolution library in C

[![CodeFactor](https://www.codefactor.io/repository/github/nmoinvaz/proxyres/badge)](https://www.codefactor.io/repository/github/nmoinvaz/proxyres) [![codecov](https://codecov.io/gh/nmoinvaz/proxyres/branch/master/graph/badge.svg?token=aBv603YArh)](https://codecov.io/gh/nmoinvaz/proxyres) [![License: MIT](https://img.shields.io/badge/license-MIT-lightgrey.svg)](https://github.com/nmoinvaz/proxyres/blob/master/LICENSE.md)

Cross-platform proxy resolution with support for Linux, macOS, & Windows.

[Read the Documentation](doc/README.md)

## Features

* Read user's proxy configuration from `IE`, `CFNetwork`, `GSettings`, `KDE user config`, or `Environment Variables`.
* Execute a PAC file using `Windows Script Host` or `JavaScriptCore`.
* Get the proxies for a URL asynchronously using `WinHTTP`, `ProxyConfiguration`, `CFNetwork`, and `Gnome3`*.
  * Or with a fallback proxy resolver featuring:
    * Support for `WPAD` using `DHCP` and `DNS`.
    * Support for PAC file execution using system scripting libraries.
* Example command line utility.

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

### Options

|Name|Description|Default|
|:-|:-|:-:|
|PROXYRES_CURL|Enables downloading PAC scripts using [curl](https://github.com/curl/curl). Without this option set, PAC scripts will only be downloaded using HTTP 1.0.|OFF|
|PROXYRES_EXECUTE|Enables support for PAC script execution. Required on Linux due to the lack of a system level proxy resolver.|ON|
|PROXYRES_BUILD_CLI|Build command line utility.|ON|
|PROXYRES_BUILD_TESTS|Build Googletest unit tests project.|ON|
|PROXYRES_CODE_COVERAGE|Build for code coverage.|OFF|

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
