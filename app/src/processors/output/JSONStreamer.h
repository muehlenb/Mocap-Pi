#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "src/model/DataModel.h"
#include "src/processors/Processor.h"

// UDP broadcast, one JSON object per datagram.
// Must run AFTER RigidBodyDetector in the processor chain.
// Marker cloud: all model.markers, including those used by rigid bodies.
// Rigid bodies: model marker definitions + found flag + pose (if found).
// No measured marker-to-body assignments are transmitted.
class JSONStreamer : public Processor {
public:
    explicit JSONStreamer(DataModel& model, unsigned short port = 5005, const std::string& destination = "255.255.255.255")
        : Processor(model), port_(port), destination_(destination) {
#ifdef _WIN32
        WSADATA data{};
        winsockReady_ = WSAStartup(MAKEWORD(2, 2), &data) == 0;
        if (!winsockReady_) return;
#endif
        socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_ == invalidSocket()) return;

        int enabled = 1;
        if (::setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&enabled), sizeof(enabled)) != 0) {
            closeSocket();
            return;
        }

        address_ = {};
        address_.sin_family = AF_INET;
        address_.sin_port = htons(port_);
        if (::inet_pton(AF_INET, destination_.c_str(), &address_.sin_addr) != 1) closeSocket();
    }

    ~JSONStreamer() {
        closeSocket();
#ifdef _WIN32
        if (winsockReady_) WSACleanup();
#endif
    }

    JSONStreamer(const JSONStreamer&) = delete;
    JSONStreamer& operator=(const JSONStreamer&) = delete;

    bool isOpen() const { return socket_ != invalidSocket(); }
    unsigned short port() const { return port_; }

    // Keeps individual cloud UDP datagrams reasonably small for typical marker counts.
    // Lower this if your network has a small MTU.
    std::size_t markersPerPacket = 25;

    void process(TickData& tick) override {
        if (!isOpen()) return;
        ++frame_;
        sendMarkerCloud(tick);
        sendRigidBodies(tick);
    }

private:
#ifdef _WIN32
    using Socket = SOCKET;
    static Socket invalidSocket() { return INVALID_SOCKET; }
    bool winsockReady_ = false;
#else
    using Socket = int;
    static Socket invalidSocket() { return -1; }
#endif
    Socket socket_ = invalidSocket();
    sockaddr_in address_{};
    unsigned short port_;
    std::string destination_;
    std::uint64_t frame_ = 0;

    void closeSocket() {
        if (!isOpen()) return;
#ifdef _WIN32
        closesocket(socket_);
#else
        ::close(socket_);
#endif
        socket_ = invalidSocket();
    }

    static void writeNumber(std::ostream& out, double value) {
        if (std::isfinite(value)) out << std::setprecision(9) << value;
        else out << "null";
    }

    template <typename Point>
    static void writeVec3(std::ostream& out, const Point& p) {
        out << '[';
        writeNumber(out, p.x);
        out << ',';
        writeNumber(out, p.y);
        out << ',';
        writeNumber(out, p.z);
        out << ']';
    }

    template <typename Quaternion>
    static void writeQuaternion(std::ostream& out, const Quaternion& q) {
        out << '[';
        writeNumber(out, q.x);
        out << ',';
        writeNumber(out, q.y);
        out << ',';
        writeNumber(out, q.z);
        out << ',';
        writeNumber(out, q.w);
        out << ']';
    }

    static std::string escape(const std::string& s) {
        std::ostringstream out;
        out << '"';
        for (unsigned char c : s) {
            switch (c) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default:
                    if (c < 0x20) {
                        const char* digits = "0123456789abcdef";
                        out << "\\u00" << digits[c >> 4] << digits[c & 15];
                    } else out << static_cast<char>(c);
            }
        }
        out << '"';
        return out.str();
    }

    template <typename Label>
    static std::string labelJson(const Label& label) {
        std::ostringstream out;
        out << label;
        return escape(out.str());
    }

    bool send(const std::string& payload) const {
        if (payload.size() > 65507 || !isOpen()) return false;
#ifdef _WIN32
        const int sent = ::sendto(socket_, payload.data(), static_cast<int>(payload.size()), 0, reinterpret_cast<const sockaddr*>(&address_), sizeof(address_));
#else
        const auto sent = ::sendto(socket_, payload.data(), payload.size(), 0, reinterpret_cast<const sockaddr*>(&address_), sizeof(address_));
#endif
        return sent >= 0 && static_cast<std::size_t>(sent) == payload.size();
    }

    void writeHeader(std::ostream& out, const char* type, const TickData& tick) const {
        out << "{\"type\":" << escape(type) << ",\"frame\":" << frame_ << ",\"timestamp\":";
        writeNumber(out, tick.timestamp);
    }

    void sendMarkerCloud(const TickData& tick) {
        const std::size_t count = model.markers.size();
        const std::size_t batchSize = std::max<std::size_t>(1, markersPerPacket);
        const std::size_t packetCount = std::max<std::size_t>(1, (count + batchSize - 1) / batchSize);
        for (std::size_t packet = 0; packet < packetCount; ++packet) {
            const std::size_t begin = packet * batchSize;
            const std::size_t end = std::min(count, begin + batchSize);
            std::ostringstream out;
            writeHeader(out, "unlabeled_markers", tick);
            out << ",\"chunk\":" << packet << ",\"chunks\":" << packetCount << ",\"total\":" << count << ",\"markers\":[";
            for (std::size_t i = begin; i < end; ++i) {
                if (i != begin) out << ',';
                out << "{\"id\":" << i << ",\"position\":";
                writeVec3(out, model.markers[i].position);
                out << ",\"obsolete\":" << (model.markers[i].obsoleteCounter != 0 ? "true" : "false") << '}';
            }
            out << "]}";
            send(out.str());
        }
    }

    void sendRigidBodies(const TickData& tick) {
        const std::size_t count = tick.allActiveRigidBodies.size();
        if (count == 0) {
            std::ostringstream out;
            writeHeader(out, "rigid_bodies", tick);
            out << ",\"total\":0,\"bodies\":[]}";
            send(out.str());
            return;
        }
        for (std::size_t i = 0; i < count; ++i) {
            const auto* body = tick.allActiveRigidBodies[i];
            if (!body) continue;
            std::ostringstream out;
            writeHeader(out, "rigid_body", tick);
            out << ",\"index\":" << i << ",\"total\":" << count << ",\"label\":" << labelJson(body->label);
            out << ",\"found\":" << (body->bodyFound ? "true" : "false");
            out << ",\"definition\":[";
            for (std::size_t j = 0; j < body->markers.size(); ++j) {
                if (j) out << ',';
                out << "{\"id\":" << j << ",\"position\":";
                writeVec3(out, body->markers[j].position);
                out << '}';
            }
            out << "],\"pose\":";
            if (body->bodyFound) {
                out << "{\"position\":";
                writeVec3(out, body->pose.getPosition());
                out << ",\"quaternion_xyzw\":";
                writeQuaternion(out, body->pose.getQuaternion());
                out << '}';
            } else out << "null";
            out << '}';
            send(out.str());
        }
    }
};
