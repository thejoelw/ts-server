#include "publisherconnection.h"

#include "stream.h"

PublisherConnection::PublisherConnection() {}

PublisherConnection::PublisherConnection(Stream *topic)
    : stream(topic)
{}
