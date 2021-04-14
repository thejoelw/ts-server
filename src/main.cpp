#include <iostream>
#include <fstream>
#include <unordered_map>

#include <uWebSockets/src/App.h>

#include "options.h"
#include "stream.h"
#include "signalhandler.h"
#include "mainloop.h"
#include "readermanager.h"
#include "subconnmanager.h"
#include "subscriberconnection.h"
#include "pubconnmanager.h"
#include "publisherconnection.h"
#include "instant.h"
#include "baseexception.h"
#include "threadmanager.h"
#include "memory.h"

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

    MainLoop::getInstance().loop = uWS::Loop::get();

    us_listen_socket_t *appListenSocket = 0;

    static constexpr unsigned int timerPeriodMs = 16;
    struct us_timer_t *timer = us_create_timer((us_loop_t *) MainLoop::getInstance().loop, 0, sizeof(&appListenSocket));
    *static_cast<us_listen_socket_t ***>(us_timer_ext(timer)) = &appListenSocket;
    us_timer_set(timer, [](struct us_timer_t *timer) {
        if (SignalHandler::getInstance().shouldExit()) {
            us_listen_socket_t *&appListenSocket = **static_cast<us_listen_socket_t ***>(us_timer_ext(timer));
            if (appListenSocket) {
                PubConnManager::getInstance().closeAll();
                SubConnManager::getInstance().closeAll();
                us_timer_close(timer);
                us_listen_socket_close(false, appListenSocket);
                appListenSocket = 0;
            }
        } else {
            ReaderManager::getInstance().tick();
            SubConnManager::getInstance().tick();

#if DEBUG_ALLOCATIONS
            static unsigned int x = 0;
            x++;
            if (x % (1024 / timerPeriodMs) == 0) {
                printSizeDist();
            }
#endif
        }
    }, timerPeriodMs, timerPeriodMs);

    uWS::App::WebSocketBehavior<SubscriberConnection> subConfig = {
        .compression = uWS::DEDICATED_COMPRESSOR_256KB,
        .maxPayloadLength = 16 * 1024 * 1024 - 16,
        .idleTimeout = 10,
        .maxBackpressure = 64 * 1024 * 1024 - 16
    };
    uWS::App::WebSocketBehavior<PublisherConnection> pubConfig = {
        .compression = uWS::DISABLED,
        .maxPayloadLength = 16 * 1024 * 1024 - 16,
        .idleTimeout = 10,
        .maxBackpressure = 1 * 1024 * 1024 - 16
    };

    subConfig.upgrade = [&streams](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, struct us_socket_context_t *socketCtx) {
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
                if (ptr.substr(0, 2) == "us") {
                    // Default
                    ptr.remove_prefix(2);
                } else if (ptr.substr(0, 2) == "ms") {
                    mul = 1000ull;
                    ptr.remove_prefix(2);
                } else if (ptr.substr(0, 1) == "s") {
                    mul = 1000ull * 1000ull;
                    ptr.remove_prefix(1);
                } else if (ptr.substr(0, 1) == "m") {
                    mul = 1000ull * 1000ull * 60;
                    ptr.remove_prefix(1);
                } else if (ptr.substr(0, 1) == "h") {
                    mul = 1000ull * 1000ull * 60 * 60;
                    ptr.remove_prefix(1);
                } else if (ptr.substr(0, 1) == "d") {
                    mul = 1000ull * 1000ull * 60 * 60 * 24;
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
                SubscriberConnection(stream, SubSpec{ .beginTime = beginTime, .endTime = endTime, .head = head, .tail = tail}),
                req->getHeader("sec-websocket-key"),
                req->getHeader("sec-websocket-protocol"),
                req->getHeader("sec-websocket-extensions"),
                socketCtx
            );
        } catch (const BadRequestException &ex) {
            res->writeStatus("400")->end(ex.what());
        }
    };

    subConfig.open = [](SubWsConn *ws) {
        SubscriberConnection *conn = ws->getUserData();
        conn->wsConn = ws;
        SubConnManager::getInstance().addConnection(conn);
    };

    subConfig.close = [](SubWsConn *ws, int /*code*/, std::string_view /*message*/) {
        SubscriberConnection *conn = static_cast<SubscriberConnection *>(ws->getUserData());
        SubConnManager::getInstance().removeConnection(conn);
    };

    pubConfig.upgrade = [&streams](uWS::HttpResponse<false> *res, uWS::HttpRequest *req, struct us_socket_context_t *socketCtx) {
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

    pubConfig.open = [](PubWsConn *ws) {
        PublisherConnection *conn = static_cast<PublisherConnection *>(ws->getUserData());
        conn->wsConn = ws;
        PubConnManager::getInstance().addConnection(conn);
    };

    pubConfig.message = [](PubWsConn *ws, std::string_view message, uWS::OpCode opCode) {
        (void) opCode;
        PublisherConnection *conn = static_cast<PublisherConnection *>(ws->getUserData());

        switch (opCode) {
        case uWS::OpCode::BINARY:
            conn->stream->publish(message.data(), message.size());
            break;

        case uWS::OpCode::TEXT:
            if (message.substr(0, 4) == "PUB ") {
                message.remove_prefix(4);
                conn->stream->publish(message.data(), message.size());
            } else if (message.substr(0, 7) == "COMMIT ") {
                message.remove_prefix(7);
                conn->stream->commit(Commit([ws, replyKey = std::string(message)]() {
                    ws->send(replyKey, uWS::OpCode::TEXT, false);
                }));
            } else {
                ws->send("ERROR Invalid command", uWS::OpCode::TEXT, false);
            }
            break;

        default:;
        }
    };

    pubConfig.close = [](PubWsConn *ws, int /*code*/, std::string_view /*message*/) {
        PublisherConnection *conn = static_cast<PublisherConnection *>(ws->getUserData());
        PubConnManager::getInstance().removeConnection(conn);
    };

    app
        .ws<SubscriberConnection>("/sub", std::move(subConfig))
        .ws<PublisherConnection>("/pub", std::move(pubConfig))
        .listen(9001, [&appListenSocket](us_listen_socket_t *listenSocket) {
            appListenSocket = listenSocket;
            if (listenSocket) {
                std::cout << "Listening on port " << 9001 << std::endl;
            }
        })
        .run();

    for (const std::pair<const std::string, Stream *> &stream : streams) {
        delete stream.second;
    }

    std::cout << "Waiting for reading threads to finish..." << std::endl;
    ReaderManager::getInstance().joinAll();

    std::cout << "Waiting for writing threads to finish..." << std::endl;
    ThreadManager::getInstance().joinAll();

    std::cout << "Ended gracefully!" << std::endl;
}
