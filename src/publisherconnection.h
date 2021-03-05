#pragma once

#include "wsconn.h"

class Stream;

class PublisherConnection {
public:
    PublisherConnection();
    PublisherConnection(Stream *topic);

    WsConn *wsConn = 0;

    Stream *stream = 0;
};
