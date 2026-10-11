# mod-ip-limiter

![banner](banner.jpg)

A module for [AzerothCore](https://www.azerothcore.org/) that limits how many players may connect from a single IP address. When the limit is exceeded, **every** player from that IP is kicked. Specific IP addresses (e.g. internet cafés, LAN parties, or your own office) can be whitelisted and exempted from the limit.

This is useful for reducing multi-boxing abuse, preventing a single host from flooding the realm, and keeping player slots available for legitimate players.

---

Not running Azerothcore? Check the [port for Trinitycore 3.3.5](https://github.com/electrobutterfly/trinitycore-ip-limiter)

------



## What It Does

While the world server is running, this module periodically scans all connected sessions:

1. Every `IPLimit.CheckInterval` milliseconds, it groups all active sessions by their remote IP address.
2. Sessions from IPs on the whitelist are ignored completely.
3. For every non-whitelisted IP, it counts the number of connected players.
4. If the count **exceeds** `IPLimit.MaxPlayersPerIP`, **all** sessions from that IP are kicked with a reason like:
   `IP limit exceeded: 5 players max per IP`
5. The whitelist is reloaded whenever the configuration is reloaded (`.reload config`).

Note that the check is a periodic scan, not a real-time rejection at login. A burst of connections may briefly exceed the limit until the next check tick.

---

## Installation

Clone the module into your AzerothCore `modules` directory (from the root of your AzerothCore source tree):

```bash
cd modules
git clone https://github.com/electrobutterfly/mod-ip-limiter.git

```



Then re-run CMake and rebuild the server:

```bash
cd ..
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/your/server
make -j$(nproc)
make install
```



After `make install`, the distribution configuration file (`conf/mod_ip_limiter.conf.dist`) is automatically copied into the module configuration directory of your server installation, typically:

```bash
<server-root>/etc/modules/mod_ip_limiter.conf.dist
```



Rename/copy it to `mod_ip_limiter.conf` (dropping the `.dist` suffix) and edit it as needed:

```bash
cp etc/modules/mod_ip_limiter.conf.dist etc/modules/mod_ip_limiter.conf
```



Restart the worldserver to load the module.

------

## Configuration

All options live in `mod_ip_limiter.conf`.

### `IPLimit.Enabled`

Enable or disable the IP limiter module.

| Value | Meaning  |
| :---- | :------- |
| `1`   | Enabled  |
| `0`   | Disabled |

Default: `0` (disabled)

> The module ships disabled by default. Set this to `1` after you have verified your `MaxPlayersPerIP` and whitelist settings.

### `IPLimit.MaxPlayersPerIP`

Maximum number of simultaneous players allowed from a single IP address. If the count **exceeds** this value, all players from that IP are kicked.

Default: `5`

Example: with `MaxPlayersPerIP = 5`, up to 5 players from one IP are fine. The 6th player triggers a kick of all 6.

### `IPLimit.CheckInterval`

How often (in milliseconds) the module scans sessions for IP limit violations. Lower values react faster to violations but use slightly more CPU; higher values are lighter but allow brief over-limit periods.

Default: `30000` (30 seconds)

### `IPLimit.Whitelist`

A comma-separated list of IP addresses that are exempt from the limit. Whitespace around entries is trimmed.

Default: `""` (empty — no IPs whitelisted)

Example:

```bash
IPLimit.Whitelist = "192.168.1.100,192.168.1.101,203.0.113.50"
```



### Example

```bash
IPLimit.Enabled          = 1
IPLimit.MaxPlayersPerIP  = 3
IPLimit.CheckInterval    = 15000
IPLimit.Whitelist        = "203.0.113.10,203.0.113.11"
```



------

## Logging

The module writes log lines about each tick, each non-whitelisted IP's session count, whitelist skips, and each kick performed.

The shipped configuration file declares a dedicated logger:

```bash
Logger.modules.iplimit = 3,Console Server
```



In the current source, however, log lines are emitted through the `server.loading` logger. If you want to silence the module, either remove the `Logger.modules.iplimit` line (it has no effect with the current code) or lower the `server.loading` logger's level in your `worldserver.conf`.

- `3` is the log level (see AzerothCore logging levels).
- `Console Server` means output goes to both the console and the server log file.

------

## Notes

- Only connected sessions with a valid `Player` object are counted. Sessions still at character selection or in the login process are ignored.
- Grouping is done purely by IP string, so two players behind the same NAT/router share a bucket. Use the whitelist for known shared connections (e.g. internet cafés, dorms, offices).
- The limit is inclusive up to `MaxPlayersPerIP`; the kick triggers only when the count is strictly greater.
- Because checks are periodic, raising `MaxPlayersPerIP` during peak hours takes effect at the next tick.
- Configuration reload (e.g. `.reload config`) re-reads the whitelist automatically.

------

## License

This module is licensed under the GNU General Public License v2 (or later). See the header comments in the source files for details.

Copyright (C) 2026 Klaus Simon
