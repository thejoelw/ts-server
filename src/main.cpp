#include <iostream>
#include <fstream>
#include <unordered_map>

#include <uWebSockets/src/App.h>

#include "stream.h"
#include "readermanager.h"
#include "connectionmanager.h"
#include "subscriberconnection.h"
#include "publisherconnection.h"
#include "instant.h"
#include "wsconn.h"
#include "baseexception.h"

int main() {
    uWS::App app = uWS::App();
    std::unordered_map<std::string, Stream *> streams;

    static constexpr unsigned int timerPeriodMs = 10;
    struct us_timer_t *timer = us_create_timer((us_loop_t *) uWS::Loop::get(), 0, 0);
    us_timer_set(timer, [](struct us_timer_t *) {
        ReaderManager::getInstance().tick();
        ConnectionManager::getInstance().tick();
    }, 0, timerPeriodMs);

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
                return Instant::now();
            };

            std::string streamKey = getQuery("stream", "default");
            Instant beginTime = parseTime(getQuery("begin", "now"));
            Instant endTime = parseTime(getQuery("end", "now"));
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
        ConnectionManager::getInstance().addConnection(conn);
        ws->subscribe("broadcast");
    };

    auto onSubClose = [](WsConn *ws, int /*code*/, std::string_view /*message*/) {
        SubscriberConnection *conn = static_cast<SubscriberConnection *>(ws->getUserData());
        ConnectionManager::getInstance().removeConnection(conn);
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

    auto onPubMessage = [&app](WsConn *ws, std::string_view message, uWS::OpCode opCode) {
        SubscriberConnection *conn = static_cast<SubscriberConnection *>(ws->getUserData());
        conn->stream->publish(Event(Instant::now(), message.data(), message.size()));
        app.publish(conn->stream->getKey(), message, uWS::OpCode::BINARY, true);
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
        .message = onPubMessage,
    }).listen(9001, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();
//    uWS::Loop::defer()
}

/*
struct Event {
    double time;
    std::size_t offset;
    std::size_t size;

    bool operator<(const Event &other) const {
        return time < other.time;
    }
    bool operator<(double otherTime) const {
        return time < otherTime;
    }
};

class Chunk {
public:
    template <typename Emitter>
    void emit(double begin, double end, Emitter emitter) const {
        std::vector<Event>::const_iterator low = std::lower_bound(events.cbegin(), events.cend(), begin);
        std::vector<Event>::const_iterator upper = std::upper_bound(events.cbegin(), events.cend(), end);
        while (low < upper) {
            emitter.emit(low->time, buffer.data() + low->offset, low->size);
            low++;
        }
    }

private:
    // Only one source for a chunk (publishers or a file reader), so we can just emit directly. NOPE.
    // Multiple publishers need to be synchronized.

    std::string buffer;
    std::vector<Event> events;
};

class Stream {
public:
    void recvEntry(const char *data, std::size_t size) {
    }

private:
    std::vector<websocketpp::connection_hdl> currentListeners;

    std::string toFilename(double min, double max) {
        std::string out(16, '\0');
        double range[2] = {min, max};
        for (unsigned int i = 0; i < 8; i++) {
            char c = reinterpret_cast<char *>(range)[i];
            auto toHex = [](unsigned int x) {
                return x < 10 ? '0' + x : 'a' + x;
            };
            out[i * 2 + 0] = toHex(c >> 4);
            out[i * 2 + 1] = toHex(c & 0xF);
        }
        return out;
    }
};

class StreamManager {
    std::unordered_map<std::string, Stream> streams;
};

class Server {
public:
    Server() {
        server.set_error_channels(websocketpp::log::elevel::all);
        server.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);

        server.init_asio();

        server.set_message_handler([this](websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg) {
            server.get_con_from_hdl(hdl)->get_uri();
            server.send(hdl, msg->get_payload(), msg->get_opcode());
        });
    }

    void run() {
        server.listen(9002);
        server.start_accept();
        server.run();
    }

private:
    websocketpp::server<websocketpp::config::asio> server;

    // Affix own timestamp
};

int main() {
    Server server;
    server.run();

//    jv args = jv_object();

//    jq_state *jq = jq_init();
//    jq_compile_args(jq, ".", args);

//    while (true) {
//        jq_start(jq, jv_object(), 0);
//        while (true) {
//            jv out = jq_next(jq);
//            if (jv_is_valid(out)) {
//                jv_dump(out, 0);
//            } else {
//                if (jv_invalid_has_msg(out)) {
//                    jv_invalid_get_msg(out);
//                }
//                break;
//            }
//        }
//    }

//    jq_teardown(&jq);

    return 0;
}
*/
