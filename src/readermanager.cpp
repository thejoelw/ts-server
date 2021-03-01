#include "readermanager.h"

#include "chunk.h"

#include "filereader.h"
#include "decompressor.h"
#include "decoder.h"
#include "queuepublisher.h"

void ReaderManager::addReader(Chunk *chunk) {
    readers.emplace_back(chunk, [](Reader *reader) {
        FileReader<Decompressor<Decoder<QueuePublisher>>> pipe(Decompressor<Decoder<QueuePublisher>>(Decoder<QueuePublisher>(QueuePublisher(reader->queue))));
        pipe.read(reader->chunk->getFilename());
    });
}

void ReaderManager::tick() {
    for (Reader &reader : readers) {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        QueueMessage msg;
        while (reader.queue.try_dequeue(msg)) {
            if (std::holds_alternative<Event>(msg)) {
                reader.chunk->recvEvent(std::get<Event>(msg));
            } else if (std::holds_alternative<Bestow>(msg)) {
                reader.chunk->recvBestow(std::get<Bestow>(msg).data);
            } else if (std::holds_alternative<Yield>(msg)) {
                break; // Don't join immediately to give the thread some time to clean up
            } else if (std::holds_alternative<Join>(msg)) {
                reader.thread.join();
                reader.chunk = 0; // Signals that this reader can be deleted

                while (!readers.empty() && readers.front().chunk == 0) {
                    readers.pop_front();
                }
                while (!readers.empty() && readers.back().chunk == 0) {
                    readers.pop_back();
                }
            } else {
                assert(false);
            }

            if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(1)) {
                break;
            }
        }
    }
}
