#pragma once

class Stream;

class PublisherConnection {
public:
    PublisherConnection();
    PublisherConnection(Stream *topic);

    Stream *stream = 0;
};
