# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses the Tup build system. Build commands:

```bash
# Initialize and build (first time)
bash third_party/update.bash
tup init
tup variant configs/*
tup

# Subsequent builds
tup

# Alternative build script
bash make.sh
```

Build variants are configured in `configs/`:
- `debug.config` - Debug build with symbols
- `release.config` - Optimized release build
- `qtc.config` - Qt Creator configuration

The project can also be built with CMake:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Running the Server

```bash
# Create data directory
mkdir /tmp/ts-server-data

# Run server (release build recommended for production)
./build-release/ts-server /tmp/ts-server-data/
```

Server runs on port 9001 by default.

## Architecture

Ts-server is a high-performance pub/sub WebSocket server for streaming timestamped data. Key architectural components:

### Core Components

- **Stream** - Central data stream abstraction managing chunks of timestamped data
- **Chunk** - Fixed-size containers of compressed data records
- **Memory** - Custom memory management for efficient chunk allocation
- **GarbageCollector** - Manages cleanup of expired data chunks

### Connection Management

- **PubConnManager/PublisherConnection** - Handles WebSocket connections for data publishing
- **SubConnManager/SubscriberConnection** - Manages subscriber connections and data delivery
- **MainLoop** - Central event loop using µWebSockets

### Data Processing

- **ReaderManager** - Manages background threads for reading historical data
- **WriterManager** - Handles background compression and disk I/O
- **ThreadManager** - Coordinates worker threads for I/O operations
- **Compressor/Decompressor** - zstd compression for efficient storage
- **Encoder/Decoder** - Binary encoding/decoding of timestamped records

### Storage

- **FileReader/FileWriter** - Disk I/O for persistent storage
- **Buffer** - Memory buffers for efficient data handling
- **Chunker** - Splits data streams into manageable chunks

Data is stored as zstd-compressed chunks with microsecond timestamps. The server handles both historical queries and live streaming seamlessly.

## WebSocket API

The server exposes two main endpoints:

- `/pub?stream=<name>` - Publishing endpoint
- `/sub?stream=<name>&begin=<time>&end=<time>&head=<limit>&jq=<query>` - Subscription endpoint

Time parameters support relative formats (e.g., `now-1h`) or absolute microsecond timestamps.

### JQ Filtering

The `/sub` endpoint supports an optional `jq` parameter for filtering and transforming output data using JQ queries:

```
ws://localhost:9001/sub?stream=my_stream&jq=.field1
ws://localhost:9001/sub?stream=my_stream&jq='select(.value > 100)'
```

If JQ processing fails or returns empty results for an event, that event is skipped.

## Dependencies

- **µWebSockets** - High-performance WebSocket library
- **zstd** - Compression library
- **libjq** - JSON processing library for filtering
- **jemalloc** (Linux) - Memory allocator
- **readerwriterqueue** - Lock-free queue for thread communication

## Code Conventions

- C++17 standard
- Header-only templates in `.h` files
- Singleton pattern used for managers (getInstance())
- RAII for resource management
- Custom exception hierarchy inheriting from BaseException