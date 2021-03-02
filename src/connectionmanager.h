#pragma once

#include <vector>

class SubscriberConnection;

class ConnectionManager {
public:
    static ConnectionManager &getInstance() {
        static ConnectionManager inst;
        return inst;
    }

    void addConnection(SubscriberConnection *conn) {
        assert(std::find(connections.cbegin(), connections.cend(), conn) == connections.cend());
        connections.push_back(conn);
    }
    void removeConnection(SubscriberConnection *conn) {
        std::vector<SubscriberConnection *>::iterator found = std::find(connections.begin(), connections.end(), conn);
        assert(found != connections.end());
        *found = connections.back();
        connections.pop_back();
    }

    void tick();

private:
    std::vector<SubscriberConnection *> connections;
};
