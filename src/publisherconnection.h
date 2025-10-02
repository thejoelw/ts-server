#pragma once

#include "uWebSockets/src/App.h"

class PublisherConnection;

class Stream;

typedef uWS::WebSocket<false, true, PublisherConnection> PubWsConn;

class PublisherConnection {
public:
  PublisherConnection();
  PublisherConnection(Stream *topic);

  PubWsConn *wsConn = 0;

  Stream *stream = 0;
};
