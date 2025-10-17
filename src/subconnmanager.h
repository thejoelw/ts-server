#pragma once

#include <vector>

class SubscriberConnection;

class SubConnManager {
public:
  static SubConnManager &getInstance() {
    static SubConnManager inst;
    return inst;
  }

  void addConnection(SubscriberConnection *conn);
  void removeConnection(SubscriberConnection *conn);

  void dispatchClose(SubscriberConnection *conn);

  void closeAll();

  // Returns the number of milliseconds to wait before the next tick
  unsigned int tick();

private:
  std::vector<SubscriberConnection *> connections;
  std::vector<SubscriberConnection *> closeQueue;
};
