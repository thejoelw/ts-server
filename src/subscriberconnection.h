#pragma once

#include <memory>
#include <string_view>

#include "event.h"
#include "jqprocessor.h"
#include "subspec.h"

#include "uWebSockets/src/App.h"

class SubscriberConnection;

class Stream;

typedef uWS::WebSocket<false, true, SubscriberConnection> SubWsConn;

class SubscriberConnection {
public:
  SubscriberConnection();
  SubscriberConnection(Stream *topic, SubSpec spec);

  SubWsConn *wsConn = 0;

  Stream *stream = 0;
  SubSpec spec;
  std::unique_ptr<JqProcessor> jqProcessor;

  std::size_t nextChunkId;
  std::size_t nextEventId;

  std::vector<std::string> emitQueue;

  void tick();

  SubWsConn::SendStatus emit(Event event);

  void dispatchClose();
};
