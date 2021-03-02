#include "writermanager.h"

#include "encoder.h"
#include "compressor.h"
#include "filewriter.h"

void WriterManager::open(const std::string &filename) {
    assert(!!queue == thread.joinable());
    if (queue) {
        queue->enqueue(Join());
        thread.detach();
    }

    queue = new moodycamel::ReaderWriterQueue<QueueMessage>();
    thread = std::thread([](const std::string &filename, moodycamel::ReaderWriterQueue<QueueMessage> *queue) {
        Encoder<Compressor<FileWriter>> pipe(std::ref(filename));

        while (true) {
            QueueMessage msg;
            while (queue->try_dequeue(msg)) {
                if (std::holds_alternative<Event>(msg)) {
                    pipe.consumeEvent(std::get<Event>(msg));
                } else if (std::holds_alternative<Bestow>(msg)) {
                    delete[] std::get<Bestow>(msg).data;
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

void WriterManager::write(Event event) {
    queue->enqueue(event);
}
