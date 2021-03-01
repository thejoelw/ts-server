#pragma once

#include <vector>

class Connection;

class ConnectionManager {
public:
    static ConnectionManager &getInstance() {
        static ConnectionManager inst;
        return inst;
    }

    void addConnection(Connection *conn) {
        assert(std::find(connections.cbegin(), connections.cend(), conn) == connections.cend());
        connections.push_back(conn);
    }
    void removeConnection(Connection *conn) {
        std::vector<Connection *>::iterator found = std::find(connections.begin(), connections.end(), conn);
        assert(found != connections.end());
        *found = connections.back();
        connections.pop_back();
    }

    void tick();

private:
    std::vector<Connection *> connections;
};
