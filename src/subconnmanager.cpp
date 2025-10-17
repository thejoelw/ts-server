#include "subconnmanager.h"

#include "stream.h"
#include "subscriberconnection.h"

void SubConnManager::addConnection(SubscriberConnection *conn) {
  assert(std::find(connections.cbegin(), connections.cend(), conn) == connections.cend());
  connections.push_back(conn);
}

void SubConnManager::removeConnection(SubscriberConnection *conn) {
  conn->stream->removeRealtimeSub(conn);

  std::vector<SubscriberConnection *>::iterator found =
      std::find(connections.begin(), connections.end(), conn);
  if (found != connections.end()) {
    *found = connections.back();
    connections.pop_back();
  }
}

void SubConnManager::dispatchClose(SubscriberConnection *conn) { closeQueue.push_back(conn); }

void SubConnManager::closeAll() {
  // Make a copy because removeConnection() could mutate the connections vector
  for (SubscriberConnection *conn : std::vector<SubscriberConnection *>(connections)) {
    conn->wsConn->end();
  }
}

unsigned int SubConnManager::tick() {
  unsigned int waitMs = static_cast<unsigned int>(-1);

  for (SubscriberConnection *conn : closeQueue) {
    conn->wsConn->end();
  }
  closeQueue.clear();

  for (SubscriberConnection *conn : connections) {
    unsigned int connWaitMs = conn->tick();
    if (connWaitMs < waitMs) {
      waitMs = connWaitMs;
    }
  }

  return waitMs;
}
