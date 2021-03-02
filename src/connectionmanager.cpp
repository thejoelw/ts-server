#include "connectionmanager.h"

#include "subscriberconnection.h"
#include "stream.h"

void ConnectionManager::tick() {
    static thread_local std::vector<SubscriberConnection *> closes;
    closes.clear();

    for (SubscriberConnection *conn : connections) {
        try {
            conn->tick();
        } catch (const Stream::UnsubscribeException &ex) {
            closes.push_back(conn);
        }
    }

    for (SubscriberConnection *conn : closes) {
        conn->wsConn->end();
    }
}
