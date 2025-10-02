#pragma once

#include <vector>

class PublisherConnection;

class PubConnManager {
public:
  static PubConnManager &getInstance() {
    static PubConnManager inst;
    return inst;
  }

  void addConnection(PublisherConnection *conn);
  void removeConnection(PublisherConnection *conn);

  void closeAll();

private:
  std::vector<PublisherConnection *> connections;
};
