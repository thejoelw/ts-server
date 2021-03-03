#include "connectionmanager.h"

#include "subscriberconnection.h"
#include "stream.h"

void ConnectionManager::addConnection(SubscriberConnection *conn) {
    assert(std::find(connections.cbegin(), connections.cend(), conn) == connections.cend());
    connections.push_back(conn);
}

void ConnectionManager::removeConnection(SubscriberConnection *conn) {
    conn->stream->unsubConnection(conn);

    std::vector<SubscriberConnection *>::iterator found = std::find(connections.begin(), connections.end(), conn);
    assert(found != connections.end());
    *found = connections.back();
    connections.pop_back();
}

void ConnectionManager::dispatchClose(SubscriberConnection *conn) {
    closeQueue.push_back(conn);
}

void ConnectionManager::closeAll() {
    // Make a copy because removeConnection() could mutate the connections vector
    for (SubscriberConnection *conn : std::vector<SubscriberConnection *>(connections)) {
        conn->wsConn->end();
    }
}

void ConnectionManager::tick() {
    for (SubscriberConnection *conn : closeQueue) {
        conn->wsConn->end();
    }
    closeQueue.clear();

    for (SubscriberConnection *conn : connections) {
        conn->tick();
    }
}
