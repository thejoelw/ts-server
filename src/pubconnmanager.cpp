#include "pubconnmanager.h"

#include "publisherconnection.h"
#include "stream.h"

void PubConnManager::addConnection(PublisherConnection *conn) {
    assert(std::find(connections.cbegin(), connections.cend(), conn) == connections.cend());
    connections.push_back(conn);
}

void PubConnManager::removeConnection(PublisherConnection *conn) {
    std::vector<PublisherConnection *>::iterator found = std::find(connections.begin(), connections.end(), conn);
    assert(found != connections.end());
    *found = connections.back();
    connections.pop_back();
}

void PubConnManager::closeAll() {
    // Make a copy because removeConnection() could mutate the connections vector
    for (PublisherConnection *conn : std::vector<PublisherConnection *>(connections)) {
        conn->wsConn->end();
    }
}
