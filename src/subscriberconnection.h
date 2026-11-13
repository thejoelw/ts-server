#pragma once

#include <memory>
#include <string_view>

#include "event.h"
#include "jqprocessor.h"
#include "messageformatter.h"
#include "subspec.h"

#include "uWebSockets/src/App.h"

class SubscriberConnection;

class Stream;

typedef uWS::WebSocket<false, true, SubscriberConnection> SubWsConn;

class SubscriberConnection {
public:
  SubscriberConnection();
  SubscriberConnection(Stream *topic, SubSpec spec, MessageFormatter messageFormatter);

  SubWsConn *wsConn = 0;

  Stream *stream = 0;
  SubSpec spec;
  MessageFormatter messageFormatter;
  std::unique_ptr<JqProcessor> jqProcessor;

  std::size_t nextChunkId;
  std::size_t nextEventId;

  std::vector<std::string> emitQueue;

  unsigned int tickDelayMs = static_cast<unsigned int>(-1);

  unsigned int tick();

  SubWsConn::SendStatus emit(Event event);

  void dispatchClose();
};
