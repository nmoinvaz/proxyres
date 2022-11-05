function FindProxyForURL(url, host) {
  if (isPlainHostName(host) || host == "127.0.0.1") {
    return "DIRECT";
  }
  if (host == "simple.com") {
    return "PROXY no-such-proxy:80";
  } else if (host == "multi.com") {
    return "HTTP some-such-proxy; HTTPS secure-some-proxy:41"
  }
  return "DIRECT";
}