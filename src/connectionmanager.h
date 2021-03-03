#pragma once

#include <vector>

class SubscriberConnection;

class ConnectionManager {
public:
    static ConnectionManager &getInstance() {
        static ConnectionManager inst;
        return inst;
    }

    void addConnection(SubscriberConnection *conn);
    void removeConnection(SubscriberConnection *conn);

    void dispatchClose(SubscriberConnection *conn);

    void closeAll();

    void tick();

private:
    std::vector<SubscriberConnection *> connections;
    std::vector<SubscriberConnection *> closeQueue;
};
