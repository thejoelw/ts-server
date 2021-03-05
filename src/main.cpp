#include <iostream>
#include <fstream>
#include <unordered_map>

#include <uWebSockets/src/App.h>

#include "options.h"
#include "stream.h"
#include "signalhandler.h"
#include "readermanager.h"
#include "subconnmanager.h"
#include "subscriberconnection.h"
#include "pubconnmanager.h"
#include "publisherconnection.h"
#include "instant.h"
#include "wsconn.h"
#include "baseexception.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./ts-server [data path]" << std::endl;
        return 1;
    }

    Options::getMutableOptions().dataPath = argv[1];
    while (Options::getMutableOptions().dataPath.back() == '/') {
        Options::getMutableOptions().dataPath.pop_back();
    }

    uWS::App app = uWS::App();
    std::unordered_map<std::string, Stream *> streams;

    us_listen_socket_t *appListenSocket = 0;

    static constexpr unsigned int timerPeriodMs = 12;
    struct us_timer_t *timer = us_create_timer((us_loop_t *) uWS::Loop::get(), 0, sizeof(&appListenSocket));
    *static_cast<us_listen_socket_t ***>(us_timer_ext(timer)) = &appListenSocket;
    us_timer_set(timer, [](struct us_timer_t *timer) {
        if (SignalHandler::getInstance().shouldExit()) {
            us_listen_socket_t *&appListenSocket = **static_cast<us_listen_socket_t ***>(us_timer_ext(timer));
            if (appListenSocket) {
                SubConnManager::getInstance().closeAll();
                PubConnManager::getInstance().closeAll();
                us_timer_close(timer);
                us_listen_socket_close(false, appListenSocket);
                appListenSocket = 0;
            }
        } else {
            ReaderManager::getInstance().tick();
            SubConnManager::getInstance().tick();
        }
    }, 1, timerPeriodMs);

    auto onSubUpgrade = [&streams](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, struct us_socket_context_t *socketCtx) {
        class BadRequestException : public BaseException {
        public:
            BadRequestException(const std::string &msg)
                : BaseException("Bad Request: " + msg)
            {}
        };

        try {
            auto getQuery = [req](const char *key, const char *def) -> std::string {
                std::string_view value = req->getQuery(key);
                return value.data() ? std::string(value) : std::string(def);
            };

            auto parseTime = [](const std::string &str) -> Instant {
                std::string_view ptr(str);

                std::uint64_t num = 0;
                if (ptr.substr(0, 3) == "now") {
                    num = Instant::now().toUint64();
                    ptr.remove_prefix(3);
                } else if (ptr.substr(0, 3) == "min") {
                    // Default
                    ptr.remove_prefix(3);
                } else if (ptr.substr(0, 3) == "max") {
                    num = static_cast<std::uint64_t>(-1);
                    ptr.remove_prefix(3);
                }

                bool neg = false;
                if (ptr.substr(0, 1) == "+" || ptr.substr(0, 1) == " ") {
                    // Default
                    ptr.remove_prefix(1);
                } else if (ptr.substr(0, 1) == "-") {
                    neg = true;
                    ptr.remove_prefix(1);
                }

                std::uint64_t inc = 0;
                while (!ptr.empty() && ptr.front() >= '0' && ptr.front() <= '9') {
                    inc *= 10;
                    inc += ptr.front() - '0';
                    ptr.remove_prefix(1);
                }

                std::uint64_t mul = 1ull;
                if (ptr.substr(0, 2) == "ms") {
                    // Default
                    ptr.remove_prefix(2);
                } else if (ptr.substr(0, 1) == "s") {
                    mul = 1000ull;
                    ptr.remove_prefix(1);
                } else if (ptr.substr(0, 1) == "m") {
                    mul = 1000ull * 60;
                    ptr.remove_prefix(1);
                } else if (ptr.substr(0, 1) == "h") {
                    mul = 1000ull * 60 * 60;
                    ptr.remove_prefix(1);
                } else if (ptr.substr(0, 1) == "d") {
                    mul = 1000ull * 60 * 60 * 24;
                    ptr.remove_prefix(1);
                }

                if (!ptr.empty()) {
                    throw BadRequestException("Could not parse time \"" + str + "\", error at \"" + std::string(ptr.substr(0, 20)) + "\"");
                }

                if (neg) {
                    num -= inc * mul;
                } else {
                    num += inc * mul;
                }

                return Instant::fromUint64(num);
            };

            std::string streamKey = getQuery("stream", "default");
            Instant beginTime = parseTime(getQuery("begin", ""));
            Instant endTime = parseTime(getQuery("end", ""));
            std::uint64_t head = std::stoull(getQuery("head", "18446744073709551615"));
            std::uint64_t tail = std::stoull(getQuery("tail", "18446744073709551615"));

            if (head != 18446744073709551615ull && tail != 18446744073709551615ull) {
                throw BadRequestException("Cannot specify both head and tail");
            }

            Stream *&stream = streams[streamKey];
            if (stream == 0) {
                stream = new Stream(streamKey);
            }

            res->template upgrade<SubscriberConnection>(
                SubscriberConnection(stream, beginTime, endTime, head, tail),
                req->getHeader("sec-websocket-key"),
                req->getHeader("sec-websocket-protocol"),
                req->getHeader("sec-websocket-extensions"),
                socketCtx
            );
        } catch (const BadRequestException &ex) {
            res->writeStatus("400")->end(ex.what());
        }
    };

    auto onSubOpen = [](WsConn *ws) {
        SubscriberConnection *conn = static_cast<SubscriberConnection *>(ws->getUserData());
        conn->wsConn = ws;
        SubConnManager::getInstance().addConnection(conn);
    };

    auto onSubClose = [](WsConn *ws, int /*code*/, std::string_view /*message*/) {
        SubscriberConnection *conn = static_cast<SubscriberConnection *>(ws->getUserData());
        SubConnManager::getInstance().removeConnection(conn);
    };

    auto onPubUpgrade = [&streams](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, struct us_socket_context_t *socketCtx) {
        class BadRequestException : public BaseException {
        public:
            BadRequestException(const std::string &msg)
                : BaseException("Bad Request: " + msg)
            {}
        };

        try {
            auto getQuery = [req](const char *key, const char *def) -> std::string {
                std::string_view value = req->getQuery(key);
                return value.data() ? std::string(value) : std::string(def);
            };

            std::string streamKey = getQuery("stream", "default");

            Stream *&stream = streams[streamKey];
            if (stream == 0) {
                stream = new Stream(streamKey);
            }

            res->template upgrade<PublisherConnection>(
                PublisherConnection(stream),
                req->getHeader("sec-websocket-key"),
                req->getHeader("sec-websocket-protocol"),
                req->getHeader("sec-websocket-extensions"),
                socketCtx
            );
        } catch (const BadRequestException &ex) {
            res->writeStatus("400")->end(ex.what());
        }
    };

    auto onPubOpen = [](WsConn *ws) {
        PublisherConnection *conn = static_cast<PublisherConnection *>(ws->getUserData());
        conn->wsConn = ws;
        PubConnManager::getInstance().addConnection(conn);
    };

    auto onPubMessage = [](WsConn *ws, std::string_view message, uWS::OpCode opCode) {
        (void) opCode;
        PublisherConnection *conn = static_cast<PublisherConnection *>(ws->getUserData());
        conn->stream->publish(message.data(), message.size());
    };

    auto onPubClose = [](WsConn *ws, int /*code*/, std::string_view /*message*/) {
        PublisherConnection *conn = static_cast<PublisherConnection *>(ws->getUserData());
        PubConnManager::getInstance().removeConnection(conn);
    };

    app.ws<SubscriberConnection>("/sub", {
        .compression = uWS::DEDICATED_COMPRESSOR_256KB,
        .maxPayloadLength = 16 * 1024 * 1024,
        .idleTimeout = 10,
        .maxBackpressure = 1 * 1024 * 1024,
        .upgrade = onSubUpgrade,
        .open = onSubOpen,
        .close = onSubClose
    }).ws<PublisherConnection>("/pub", {
        .compression = uWS::DEDICATED_COMPRESSOR_256KB,
        .maxPayloadLength = 16 * 1024 * 1024,
        .idleTimeout = 10,
        .maxBackpressure = 1 * 1024 * 1024,
        .upgrade = onPubUpgrade,
        .open = onPubOpen,
        .message = onPubMessage,
        .close = onPubClose
    }).listen(9001, [&appListenSocket](us_listen_socket_t *listenSocket) {
        appListenSocket = listenSocket;
        if (listenSocket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();

    for (const std::pair<std::string, Stream *> &stream : streams) {
        delete stream.second;
    }

    std::cout << "Ended gracefully!" << std::endl;
}
