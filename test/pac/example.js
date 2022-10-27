function FindProxyForURL(url, host) {
  if (isPlainHostName(host) || host == "127.0.0.1") {
    return "DIRECT";
  }
  if (host == "example1.com") {
    return "PROXY no-such-proxy:80";
  }
  return "DIRECT";
}