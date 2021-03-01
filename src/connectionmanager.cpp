#include "connectionmanager.h"

#include "connection.h"

void ConnectionManager::tick() {
    for (Connection *conn : connections) {
        conn->tick();
    }
}
