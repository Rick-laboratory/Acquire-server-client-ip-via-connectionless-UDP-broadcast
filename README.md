# UDP Peer Discovery

A pair of Windows C++ examples that discover a peer's IPv4 endpoint through a short UDP request/reply exchange. The client broadcasts a four-byte token; the server returns the token to the packet's source address. Both sides then have a peer endpoint available for their application's next step.

This project performs discovery only. It does not establish the later TCP connection, authenticate the discovered peer, or encrypt traffic.

## Build

Requirements: Windows, CMake 3.16+, and a C++17 compiler with Windows headers (MSVC or MinGW-w64).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For MinGW-w64 with Ninja, add `-G Ninja`. Visual Studio executables are in `build/Release/`; Ninja executables are in `build/`.

## Try it locally

In one terminal, start the server:

```powershell
.\build\discovery-server.exe 9009 127.0.0.1
```

Within five seconds, run the client in another terminal:

```powershell
.\build\discovery-client.exe 127.0.0.1 9009
```

For LAN discovery, run both executables without arguments on their respective machines. The server listens on `0.0.0.0:9009`; the client sends to `255.255.255.255:9009`. Network routing and firewall rules determine whether broadcasts arrive.

Each program exits after discovering one peer. The client retries at most once every 500 ms; both examples have a five-second deadline. `--help` prints arguments. Exit codes are `0` for success, `1` for timeout/socket errors, and `2` for invalid input.

## Code guide

| File | Responsibility |
| --- | --- |
| `discovery.hpp` | Scoped Winsock/socket ownership, explicit token bytes, bounded receiving, deadlines and endpoint formatting |
| `Inline_IPv4_Broadcast_UDP/Inline_IPv4_Broadcast_UDP.h` | Client retries and response validation |
| `Inline_IPv4_Receiver/Inline_IPv4_Receiver.h` | Server validation and one reply to a valid request |
| Each directory's `main.cpp` | A short runnable example |
| `tests/discovery_test.cpp` | Loopback handshake, malformed-packet and timeout checks |

The original class and method names remain available. `Broadcast()` and `Receive()` now return a `discovery::Peer` containing `address` and `port`; existing callers may ignore the return value. Constructors allow explicit ports, timeouts and bind addresses. Server port `0` requests an ephemeral port, available through `Port()`.

## Protocol and limits

The wire token is exactly `B5 00 0B 00`, preserving the original Windows little-endian representation of `0x000B00B5`. Request and response use the same four bytes. Empty, short, wrong and oversized packets are ignored. The client accepts a response from the requested server port; when a specific unicast address is provided, that address must also match.

This is a minimal discovery protocol without request identifiers or authentication. Multiple servers may respond; the client returns the first valid response. The reported client port is its ephemeral **UDP** source port, not an advertised TCP service. Limited broadcast is the supported discovery mode; use a specific unicast address for local tests.

Tests use only loopback and ephemeral ports. They also send an oversized packet beginning with the valid token to verify that a truncated prefix is never accepted as a complete message.

## License

This repository does not currently include a license file.
