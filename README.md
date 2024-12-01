# Ts-server

Ts-server is a pub/sub WebSocket server for streaming data. It's simple, yet fast and effective.

## Usage

Building:

```sh
git clone git@github.com:thejoelw/ts-server.git
cd ts-server
bash third_party/update.bash
tup init
tup variant configs/*
tup
```

Create a data storage directory and start the server:

```sh
mkdir /tmp/ts-server-data
./build-release/ts-server /tmp/ts-server-data/
```

You can use any ws client to interact with it. Here, we are using the excellent [websocat](https://github.com/vi/websocat):

```sh
# Pub
echo 'entry 1
entry 2
entry 3' | websocat --binary 'line2msg:-' 'line2msg:ws://localhost:9001/pub?stream=my_stream'

# Sub
websocat 'ws://localhost:9001/sub?stream=my_stream&begin=now-1h&end=now+1h'
```

## Features

Ts-server is data agnostic; each record contains only a microsecond timestamp and an array of raw bytes. It persists zstd-compressed data and all compression and file io is done off the main thread. It uses [µWebSockets](https://github.com/uNetworking/uWebSockets) for networking.

Query parameters:
- `begin` and `end`; can be a relative timestamp like the example above, or an absolute microsecond timestamp (for example, `begin=1712840700000000`).
- `head` limits the number of records returned.

Ts-server serves historical data and live data equally well. For example, the query `ws://localhost:9001/sub?stream=my_stream&begin=now-1h&end=now+1h` will first return all records over the last hour, then stream new records as they are published for the next hour.
