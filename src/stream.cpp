#include "stream.h"

#include <filesystem>

#include "options.h"
#include "subscriberconnection.h"

Stream::Stream(const std::string &key)
    : key(key)
{
    std::vector<std::uint64_t> entries;
    for (const auto &entry : std::filesystem::directory_iterator(Options::getOptions().dataPath)) {
        std::pair<bool, std::uint64_t> parsed = Chunk::parseFilename(entry.path().string(), key);
        if (parsed.first) {
            entries.push_back(parsed.second);
        }
    }

    std::sort(entries.begin(), entries.end());

    for (std::uint64_t beginTime : entries) {
        chunks.emplace_back(this, Instant::fromUint64(beginTime), true);
    }
}

std::size_t Stream::getInitChunkId(Instant beginTime) {
    std::deque<Chunk>::const_iterator found = std::upper_bound(chunks.cbegin(), chunks.cend(), beginTime, [](Instant t0, const Chunk &t1) { return t0 < t1.getBeginTime(); });
    if (found != chunks.cbegin()) { found--; }
    return found - chunks.cbegin();
}

void Stream::tick(SubscriberConnection &conn) {
    // TODO: Handle tail here - go backwards from end instead of forwards

    if (conn.nextChunkId < chunks.size()) {
        chunks[conn.nextChunkId].tick(conn);
    } else {
        if (Instant::now() >= conn.endTime) {
            conn.dispatchClose();
        } else if (conn.nextChunkId != static_cast<std::uint32_t>(-1)) {
            realtimeSubscribers.push_back(&conn);
            conn.nextChunkId = static_cast<std::uint32_t>(-1);
        }
    }
}

struct ChunkedAllocator {
    static constexpr std::size_t minSize = 64 * 1024 * 1024;

    std::shared_ptr<char> mem;
    std::size_t remainingSize = 0;
};

void Stream::publish(const char *data, std::size_t size) {
    Instant now = Instant::now();

    static thread_local ChunkedAllocator allocator;

    bool needsNewChunk = chunks.empty() || chunks.back().getStatus() != Chunk::Status::Live || chunks.back().getNumEvents() >= 1024 * 1024;
    bool needsNewMem = size > allocator.remainingSize;

    if (needsNewChunk) {
        if (!chunks.empty()) {
            chunks.back().onBestow(allocator.mem);
            writerManager.onBestow(allocator.mem);
            if (writerManager.isOpen()) {
                writerManager.close();
            }
        }

        Chunk &chunk = chunks.emplace_back(this, now, false);
        writerManager.open(chunk.getFilename());
    }

    if (needsNewMem) {
        if (!needsNewChunk) {
            chunks.back().onBestow(allocator.mem);
            writerManager.onBestow(std::move(allocator.mem));
        }

        allocator.remainingSize = std::max(size, ChunkedAllocator::minSize);
        allocator.mem = std::shared_ptr<char>(new char[allocator.remainingSize], std::default_delete<char[]>());
    }

    assert(size <= allocator.remainingSize);
    allocator.remainingSize -= size;
    char *dst = allocator.mem.get() + allocator.remainingSize;
    std::copy_n(data, size, dst);

    Event event(now, dst, size);
    chunks.back().onEvent(event);
    writerManager.onEvent(event);

    std::vector<SubscriberConnection *>::iterator it = realtimeSubscribers.begin();
    while (it != realtimeSubscribers.end()) {
        SubscriberConnection *conn = *it++;
        conn->emit(event);
    }
}

void Stream::unsubConnection(SubscriberConnection *conn) {
    std::vector<SubscriberConnection *>::iterator found = std::find(realtimeSubscribers.begin(), realtimeSubscribers.end(), conn);
    if (found != realtimeSubscribers.end()) {
        *found = realtimeSubscribers.back();
        realtimeSubscribers.pop_back();
    }
}
