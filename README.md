# ding

`ding` is a command-line utility for measuring DNS lookup latency against a
specific DNS server, similar in spirit to `ping`.

Instead of sending ICMP packets, `ding` repeatedly performs DNS lookups using
the DNS server provided as an argument and reports response times, packet loss,
and round-trip-time statistics.

## Installation

Clone the repository:

```bash
git clone https://github.com/splitnines/ding.git
cd ding
```

Build `ding` from source:

```bash
make
```

This creates the executable at:

```bash
build/ding
```

You can run it directly:

```bash
./build/ding 1.1.1.1
```

To install it system-wide:

```bash
sudo make install
```

By default, this installs `ding` to `/usr/local/bin`.

To install it somewhere else, override `PREFIX`:

```bash
make install PREFIX="$HOME/.local"
```

Make sure the install location is in your `PATH`:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

To remove build artifacts:

```bash
make clean
```

## Usage

```bash
ding <dns server> [options]
```

Options:

```text
-n, --name <hostname>        host name to look up (default: google.com)
-c, --count <count>          stop after <count> replies
-t, --timeout <timeout>      time, in seconds, to wait for a response
-i, --interval <interval>    time, in seconds, between requests
-q, --quiet                  quiet output
-s, --save [path/to/file]    save summary to file
-h, --help                   print help and exit
-V, --version                print version and exit
```

## Examples

Query Cloudflare DNS five times:

```bash
ding 1.1.1.1 -c5
```

Look up a custom hostname using Google DNS:

```bash
ding 8.8.8.8 -n example.com
```

Use a custom interval and timeout:

```bash
ding 1.1.1.1 -i0.025 -t0.025
```

Run five queries and save the summary to a file:

```bash
ding 8.8.8.8 -c5 -s results.csv
```

## Example output

```text
DNS server: 1.1.1.1 (one.one.one.one) DNS lookup: google.com
response from 1.1.1.1: name=google.com sequence=1 time=14.680 ms
response from 1.1.1.1: name=google.com sequence=2 time=15.568 ms
response from 1.1.1.1: name=google.com sequence=3 time=14.845 ms
response from 1.1.1.1: name=google.com sequence=4 time=16.401 ms
response from 1.1.1.1: name=google.com sequence=5 time=15.565 ms

--- 1.1.1.1 ding statistics ---
5 packets transmitted, 5 received, 0.00% packet loss, time 77ms
rtt min/avg/max/mdev 14.680/15.412/16.401/0.614 ms
```

## Output fields

- `sequence`: the request number
- `time`: DNS lookup response time in milliseconds
- `packet loss`: percentage of DNS requests that did not receive a response
- `rtt min/avg/max/mdev`: minimum, average, maximum, and mean deviation of response times

## OS support

`ding` has currently only been built and tested on Linux.

Other Unix-like systems may work, but they are not officially supported yet.
Windows is not currently supported.
