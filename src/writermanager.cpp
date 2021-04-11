#include "writermanager.h"

#include "encoder.h"
#include "compressor.h"
#include "buffer.h"
#include "chunker.h"
#include "filewriter.h"
#include "mainloop.h"
#include "threadmanager.h"

static constexpr Instant::duration defaultFlushDelay = std::chrono::minutes(7);
//static constexpr Instant::duration defaultFlushDelay = std::chrono::seconds(10);

WriterManager::~WriterManager() {
    if (isOpen()) {
        close();
    }
}

void WriterManager::open(const std::string &filename) {
    assert(!isOpen());

    queue = new moodycamel::BlockingReaderWriterQueue<QueueMessage>();
    thread = std::thread([](const std::string &filename, moodycamel::BlockingReaderWriterQueue<QueueMessage> *queue) {
        {
            std::cout << "Starting new thread " << std::this_thread::get_id() << " writing to " << filename << "..." << std::endl;

            std::uint64_t messageCount = 0;
            std::uint64_t eventCount = 0;

            {
                Encoder<Buffer<Compressor<Chunker<FileWriter, 64 * 1024 * 1024>>, 64 * 1024 * 1024>> pipe(std::ref(filename));

                std::chrono::steady_clock::time_point flushTime = std::chrono::steady_clock::now() + defaultFlushDelay;

                QueueMessage msg;
                while (true) {
                    std::chrono::steady_clock::duration wait = flushTime - std::chrono::steady_clock::now();
                    if (wait.count() > 0 && queue->wait_dequeue_timed(msg, wait)) {
                        messageCount++;
                        if (std::holds_alternative<Event>(msg)) {
                            eventCount++;
                            pipe.onEvent(std::get<Event>(msg));
                        } else if (std::holds_alternative<Commit>(msg)) {
                            pipe.onFlush([onFlush = std::move(std::get<Commit>(msg).onFlush)]() mutable {
                                MainLoop::getInstance().loop->defer(std::move(onFlush));
                            });
                            flushTime = std::chrono::steady_clock::now() + defaultFlushDelay;
                        } else if (std::holds_alternative<Bestow<std::unique_ptr<char[]>>>(msg)) {
                            pipe.onBestow(std::move(std::get<Bestow<std::unique_ptr<char[]>>>(msg).mem));
                        } else if (std::holds_alternative<Bestow<std::shared_ptr<char>>>(msg)) {
                            pipe.onBestow(std::move(std::get<Bestow<std::shared_ptr<char>>>(msg).mem));
                        } else if (std::holds_alternative<Bestow<std::vector<char>>>(msg)) {
                            pipe.onBestow(std::move(std::get<Bestow<std::vector<char>>>(msg).mem));
                        } else if (std::holds_alternative<Join>(msg)) {
                            delete queue;
                            break;
                        } else {
                            assert(false);
                        }
                    } else {
                        std::cout << "Flushing thread " << std::this_thread::get_id() << " (processed " << messageCount << " messages)..." << std::endl;
                        pipe.onFlush([](){});
                        std::cout << "Flushed thread " << std::this_thread::get_id() << " (processed " << messageCount << " messages)" << std::endl;
                        flushTime = std::chrono::steady_clock::now() + defaultFlushDelay;
                    }
                }
            }
            std::cout << "Ended thread " << std::this_thread::get_id() << " (processed " << messageCount << " messages and " << eventCount << " events)" << std::endl;
        }
    }, filename, queue);
}

void WriterManager::close() {
    assert(isOpen());

    std::cout << "Enqueueing join of thread " << thread.get_id() << "..." << std::endl;

    queue->enqueue(Join());
    queue = 0; // Will be deleted by thread
    ThreadManager::getInstance().enqueueJoin(std::move(thread));
}

void WriterManager::onEvent(Event event) {
    assert(queue);
    queue->enqueue(std::move(event));
}

void WriterManager::onCommit(Commit commit) {
    assert(queue);
    queue->enqueue(std::move(commit));
}
