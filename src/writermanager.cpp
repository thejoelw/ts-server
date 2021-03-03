#include "writermanager.h"

#include "encoder.h"
#include "compressor.h"
#include "filewriter.h"

WriterManager::~WriterManager() {
    close();
}

void WriterManager::open(const std::string &filename) {
    close();

    queue = new moodycamel::ReaderWriterQueue<QueueMessage>();
    thread = std::thread([](const std::string &filename, moodycamel::ReaderWriterQueue<QueueMessage> *queue) {
        Encoder<Compressor<FileWriter>> pipe(std::ref(filename));

        while (true) {
            QueueMessage msg;
            while (queue->try_dequeue(msg)) {
                if (std::holds_alternative<Event>(msg)) {
                    pipe.onEvent(std::get<Event>(msg));
                } else if (std::holds_alternative<Bestow<std::unique_ptr<char[]>>>(msg)) {
                    pipe.onBestow(std::move(std::get<Bestow<std::unique_ptr<char[]>>>(msg).mem));
                } else if (std::holds_alternative<Bestow<std::shared_ptr<char>>>(msg)) {
                    pipe.onBestow(std::move(std::get<Bestow<std::shared_ptr<char>>>(msg).mem));
                } else if (std::holds_alternative<Bestow<std::vector<char>>>(msg)) {
                    pipe.onBestow(std::move(std::get<Bestow<std::vector<char>>>(msg).mem));
                } else if (std::holds_alternative<Join>(msg)) {
                    delete queue;
                    return;
                } else {
                    assert(false);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }, filename, queue);
}

void WriterManager::close() {
    assert(!!queue == thread.joinable());
    if (queue) {
        queue->enqueue(Join());
        queue = 0;
        thread.join();
    }
}

void WriterManager::onEvent(Event event) {
    assert(queue);
    queue->enqueue(event);
}
