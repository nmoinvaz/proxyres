# proxyres

## API Overview

|Class|Description|
|-|:-|
|[proxy_config](proxy_config.md)|Read the user's proxy configuration.|
|[proxy_execute](proxy_execute.md)|Executes a Proxy Auto-Configuration (PAC) script containing the JavasScript function `FindProxyForURL` for a particular URL to determine its proxies.|
|[proxy_resolver](proxy_resolver.md)|Resolves proxies for a given URL based on the operating system's proxy configuration.|

## FindProxyForURL

Some proxy resolvers do not have support for specifying anything other than `DIRECT`, `PROXY`, and `SOCKS` as a type when returning from `FindProxyForURL`.

Specifically, both macOS and Windows do not support:
 * `HTTP host:port`
 * `HTTPS host:port`
 * `SOCKS4 host:port`
 * `SOCKS5 host:port`

Only the posix resolver in proxyres can handle these types when returning from `FindProxyForURL`.

For more information on PAC scripts can be found in Mozilla's [documentation](https://developer.mozilla.org/en-US/docs/Web/HTTP/Proxy_servers_and_tunneling/Proxy_Auto-Configuration_PAC_file).

## Using with CMake

Run the following git commands to add a new submodule to your repository:

```bash
git submodule add git@github.com:nmoinvaz/proxyres third-party/proxyres
git submodule update --init
```

Add thefollowing to your cmake:

```cmake
add_subdirectory(third-party/proxyres proxyres EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} proxyres)
```

## Using with cURL

Since `proxy_resolver` can return multiple proxies, each proxy in the list must be attempted. A direct connection should only be attempted if `direct://` was returned.

It is also import to check cURL's feature list to ensure that HTTPS proxies are supported. It is possible to build `cURL` without HTTPS proxy support on some platforms. On such platforms it may be preferrible to change the proxy URL from HTTPS to HTTP:
```c
static curl_version_info_data *version_info = curl_version_info(CURLVERSION_NOW);
// cURL is not built with HTTPS proxy support so just use HTTP proxy
if ((version_info->features & CURL_VERSION_HTTPS_PROXY) == 0) {
    memmove(proxy, proxy + 1, strlen(proxy) + 1);
    memcpy(proxy, "http:", MIN(5, strlen(proxy)));
}
```

## Linking with V8

If you are using a project that also uses V8, it is necessary to link JavaScriptCoreGTK before linking against V8. This will prevent any conflicts when JavaScriptCoreGTK is loaded dynamically at run-time.