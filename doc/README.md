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
