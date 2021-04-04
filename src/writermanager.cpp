#include "writermanager.h"

#include "encoder.h"
#include "compressor.h"
#include "buffer.h"
#include "filewriter.h"
#include "mainloop.h"
#include "dbparseexception.h"

static constexpr Instant::duration defaultFlushDelay = std::chrono::minutes(5);

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
//            Encoder<Compressor<FileWriter>> pipe(std::ref(filename));
            Encoder<Buffer<Compressor<Buffer<FileWriter, 64 * 1024 * 1024>>, 4 * 1024 * 1024>> pipe(std::ref(filename));

            std::chrono::steady_clock::time_point flushTime = std::chrono::steady_clock::now() + defaultFlushDelay;

            QueueMessage msg;
            while (true) {
                std::chrono::steady_clock::duration wait = flushTime - std::chrono::steady_clock::now();
                if (wait.count() > 0 && queue->wait_dequeue_timed(msg, wait)) {
                    if (std::holds_alternative<Event>(msg)) {
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
                    pipe.onFlush([](){});
                    flushTime = std::chrono::steady_clock::now() + defaultFlushDelay;
                }
            }
        }

        if (!DbParseException::getStore().empty()) {
            std::cerr << "Have parse exceptions from file " << filename << std::endl;
            for (const DbParseException &ex : DbParseException::getStore()) {
                std::cerr << "  " << ex.what() << std::endl;
            }
        }
    }, filename, queue);
}

void WriterManager::close() {
    assert(isOpen());

    queue->enqueue(Join());
    queue = 0; // Will be deleted by thread
    thread.join();
}

void WriterManager::onEvent(Event event) {
    assert(queue);
    queue->enqueue(std::move(event));
}

void WriterManager::onCommit(Commit commit) {
    assert(queue);
    queue->enqueue(std::move(commit));
}
