#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../core/cube.hpp"

namespace local_api {
using Clock = std::chrono::steady_clock;
constexpr unsigned short port = 8765;
constexpr std::size_t maxHeaders = 8192;
constexpr std::size_t maxBody = 8192;
constexpr std::size_t maxSessions = 256;
constexpr auto requestDeadline = std::chrono::seconds(2);
constexpr auto sessionTtl = std::chrono::minutes(30);

struct Failure { int status; const char* code; };
[[noreturn]] void invalid() { throw Failure{400, "INVALID_REQUEST"}; }
std::string lower(std::string value) {
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}
std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t") - first + 1);
}

// A deliberately narrow JSON reader: bounded ASCII objects, arrays, integer,
// boolean and string values. Unsupported number forms cannot enter the core.
struct Json {
    enum Kind { Object, Array, String, Integer, Boolean } kind;
    std::map<std::string, Json> object;
    std::vector<Json> array;
    std::string string;
    int integer = 0;
    bool boolean = false;
    explicit Json(Kind type) : kind(type) {}
};
class JsonReader {
    const std::string& input;
    std::size_t offset = 0;
    unsigned nodes = 0;
    void spaces() { while (offset < input.size() && (input[offset] == ' ' || input[offset] == '\t' || input[offset] == '\n' || input[offset] == '\r')) ++offset; }
    bool take(char c) { spaces(); if (offset < input.size() && input[offset] == c) { ++offset; return true; } return false; }
    std::string string() {
        if (!take('"')) invalid();
        std::string result;
        while (offset < input.size()) {
            unsigned char c = static_cast<unsigned char>(input[offset++]);
            if (c == '"') return result;
            if (c < 32 || c > 126) invalid();
            if (c == '\\') {
                if (offset == input.size()) invalid();
                c = static_cast<unsigned char>(input[offset++]);
                if (c == '"' || c == '\\' || c == '/') result += static_cast<char>(c);
                else if (c == 'b') result += '\b';
                else if (c == 'f') result += '\f';
                else if (c == 'n') result += '\n';
                else if (c == 'r') result += '\r';
                else if (c == 't') result += '\t';
                else if (c == 'u') {
                    if (offset + 4 > input.size()) invalid();
                    unsigned number = 0;
                    for (unsigned i = 0; i < 4; ++i) {
                        const unsigned char digit = static_cast<unsigned char>(input[offset++]);
                        number *= 16;
                        if (digit >= '0' && digit <= '9') number += digit - '0';
                        else if (digit >= 'a' && digit <= 'f') number += digit - 'a' + 10;
                        else if (digit >= 'A' && digit <= 'F') number += digit - 'A' + 10;
                        else invalid();
                    }
                    if (number < 32 || number > 126) invalid();
                    result += static_cast<char>(number);
                } else invalid();
            } else result += static_cast<char>(c);
            if (result.size() > 128) invalid();
        }
        invalid();
    }
    Json value(unsigned depth) {
        if (++nodes > 128 || depth > 5) invalid();
        spaces();
        if (offset == input.size()) invalid();
        if (take('{')) {
            Json result(Json::Object);
            if (take('}')) return result;
            do {
                const auto key = string();
                if (!take(':')) invalid();
                auto entry = value(depth + 1);
                if (!result.object.emplace(key, std::move(entry)).second) invalid();
                if (take('}')) return result;
            } while (take(','));
            invalid();
        }
        if (take('[')) {
            Json result(Json::Array);
            if (take(']')) return result;
            do {
                if (result.array.size() == 9) invalid();
                result.array.push_back(value(depth + 1));
                if (take(']')) return result;
            } while (take(','));
            invalid();
        }
        if (input[offset] == '"') { Json result(Json::String); result.string = string(); return result; }
        if (input.compare(offset, 4, "true") == 0) { offset += 4; Json result(Json::Boolean); result.boolean = true; return result; }
        if (input.compare(offset, 5, "false") == 0) { offset += 5; return Json(Json::Boolean); }
        if (input[offset] >= '0' && input[offset] <= '9') {
            Json result(Json::Integer);
            const auto start = offset;
            while (offset < input.size() && input[offset] >= '0' && input[offset] <= '9') {
                if (offset - start >= 4) invalid();
                result.integer = result.integer * 10 + input[offset++] - '0';
            }
            if (offset - start > 1 && input[start] == '0') invalid();
            return result;
        }
        invalid();
    }
public:
    explicit JsonReader(const std::string& source) : input(source) {}
    Json read() { auto result = value(0); spaces(); if (offset != input.size()) invalid(); return result; }
};

const Json& field(const Json& object, const char* key, Json::Kind kind) {
    const auto found = object.object.find(key);
    if (object.kind != Json::Object || found == object.object.end() || found->second.kind != kind) invalid();
    return found->second;
}
struct Input { std::string sessionId; int phase; int position; };
Input validate(const std::string& body) {
    const auto root = JsonReader(body).read();
    if (root.kind != Json::Object || root.object.size() != 4) invalid();
    if (field(root, "scenarioId", Json::String).string != "choice-grid") invalid();
    auto session = lower(field(root, "sessionId", Json::String).string);
    if (session.size() != 36) invalid();
    for (std::size_t i = 0; i < session.size(); ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) { if (session[i] != '-') invalid(); }
        else if (!((session[i] >= '0' && session[i] <= '9') || (session[i] >= 'a' && session[i] <= 'f'))) invalid();
    }
    const int phase = field(root, "phase", Json::Integer).integer;
    if (phase < 1 || phase > 3) invalid();
    const auto& decisions = field(root, "decisions", Json::Array).array;
    if (decisions.empty()) invalid();
    std::array<bool, 9> seen{};
    int selected = 0;
    for (const auto& decision : decisions) {
        if (decision.kind != Json::Object || decision.object.size() != 3) invalid();
        const int position = field(decision, "position", Json::Integer).integer;
        const bool active = field(decision, "selected", Json::Boolean).boolean;
        const int value = field(decision, "value", Json::Integer).integer;
        if (position < 1 || position > 9 || seen[position - 1] || value != (active ? 1 : 0)) invalid();
        seen[position - 1] = true;
        if (active) { if (selected != 0) invalid(); selected = position; }
    }
    if (selected == 0) invalid();
    return {session, phase, selected};
}

struct LocalSession {
    scenarys::Run engine;
    std::array<int, 3> accepted{};
    std::array<std::string, 3> replies{};
    Clock::time_point lastUsed = Clock::now();
};
std::map<std::string, LocalSession> sessions;
// Public DTO: only selections already stored by the engine, 1-based.
std::string measurementsJson(const scenarys::Run& run) {
    std::string json = "[";
    for (const auto& item : run.measurements()) {
        if (json.size() > 1) json += ',';
        json += "{\"phase\":" + std::to_string(item.phase) + ",\"row\":" + std::to_string(item.row)
            + ",\"column\":" + std::to_string(item.column) + "}";
    }
    return json + "]";
}
std::string evaluate(const Input& input) {
    const auto now = Clock::now();
    for (auto it = sessions.begin(); it != sessions.end();) {
        if (now - it->second.lastUsed >= sessionTtl) it = sessions.erase(it);
        else ++it;
    }
    auto existing = sessions.find(input.sessionId);
    if (existing == sessions.end()) {
        if (input.phase != 1) throw Failure{404, "SESSION_NOT_FOUND"};
        if (sessions.size() >= maxSessions) throw Failure{503, "SERVICE_BUSY"};
        existing = sessions.emplace(input.sessionId, LocalSession{}).first;
        existing->second.engine.start();
    }
    auto& state = existing->second;
    const auto slot = static_cast<std::size_t>(input.phase - 1);
    if (state.accepted[slot] != 0) {
        if (state.accepted[slot] != input.position) throw Failure{409, "SESSION_CONFLICT"};
        state.lastUsed = now;
        return state.replies[slot];
    }
    if (state.engine.finished || state.engine.currentPhase + 1 != static_cast<std::size_t>(input.phase)) throw Failure{409, "SESSION_CONFLICT"};
    // Public grid position is input encoding only; all progression belongs to C++.
    if (!state.engine.select((input.position - 1) / scenarys::columnCount, (input.position - 1) % scenarys::columnCount))
        throw Failure{500, "INTERNAL_ERROR"};
    const bool complete = state.engine.finished;
    const std::string response = "{\"ok\":true,\"result\":{\"sessionId\":\"" + input.sessionId
        + "\",\"phase\":" + std::to_string(input.phase) + ",\"status\":\""
        + (complete ? "completed" : "phase-complete") + "\",\"nextPhase\":"
        + (complete ? "null" : std::to_string(state.engine.currentPhase + 1))
        + (complete ? ",\"measurements\":" + measurementsJson(state.engine) : std::string()) + "}}";
    state.accepted[slot] = input.position;
    state.replies[slot] = response;
    state.lastUsed = now;
    return response;
}

void waitReady(SOCKET socket, bool write, Clock::time_point deadline) {
    const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - Clock::now()).count();
    if (remaining <= 0) throw Failure{408, "REQUEST_TIMEOUT"};
    timeval wait{static_cast<long>(remaining / 1000000), static_cast<long>(remaining % 1000000)};
    fd_set selected;
    FD_ZERO(&selected);
    FD_SET(socket, &selected);
    const int result = select(0, write ? nullptr : &selected, write ? &selected : nullptr, nullptr, &wait);
    if (result == 0) throw Failure{408, "REQUEST_TIMEOUT"};
    if (result == SOCKET_ERROR) invalid();
}
std::string receive(SOCKET socket, Clock::time_point deadline, std::size_t maximum) {
    waitReady(socket, false, deadline);
    char buffer[2048];
    const int length = recv(socket, buffer, static_cast<int>(std::min(maximum, sizeof(buffer))), 0);
    if (length <= 0) invalid();
    return {buffer, static_cast<std::size_t>(length)};
}
struct Request { std::string method; std::string endpoint; std::map<std::string, std::string> headers; std::string body; std::size_t length = 0; };
Request headers(SOCKET socket, Clock::time_point deadline) {
    std::string data;
    std::size_t boundary = std::string::npos;
    while ((boundary = data.find("\r\n\r\n")) == std::string::npos) {
        if (data.size() >= maxHeaders) invalid();
        data += receive(socket, deadline, maxHeaders - data.size());
    }
    Request result;
    const auto firstLine = data.find("\r\n");
    std::istringstream line(data.substr(0, firstLine));
    std::string protocol, extra;
    if (!(line >> result.method >> result.endpoint >> protocol) || (line >> extra)
        || (protocol != "HTTP/1.1" && protocol != "HTTP/1.0")) invalid();
    if (result.endpoint.size() > 64) invalid();
    std::size_t offset = firstLine + 2;
    while (offset < boundary) {
        const auto end = data.find("\r\n", offset);
        const auto colon = data.find(':', offset);
        if (colon == std::string::npos || colon >= end || colon == offset) invalid();
        const std::string name = lower(data.substr(offset, colon - offset));
        for (const char c : name) if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '-') invalid();
        const std::string value = trim(data.substr(colon + 1, end - colon - 1));
        for (const unsigned char c : value) if ((c < 32 && c != '\t') || c > 126) invalid();
        if (!result.headers.emplace(name, value).second) invalid();
        offset = end + 2;
    }
    if (result.headers.count("transfer-encoding") || result.headers.count("expect")) invalid();
    if (result.headers.count("host") == 0 || (result.headers.at("host") != "127.0.0.1:8765" && result.headers.at("host") != "localhost:8765")) invalid();
    if (result.headers.count("content-length")) {
        const auto& length = result.headers.at("content-length");
        if (length.empty() || length.size() > 5) invalid();
        for (const char c : length) {
            if (c < '0' || c > '9') invalid();
            result.length = result.length * 10 + static_cast<unsigned>(c - '0');
            if (result.length > maxBody) invalid();
        }
    }
    result.body = data.substr(boundary + 4);
    if (result.body.size() > result.length) invalid();
    return result;
}
bool equalToken(const std::string& supplied, const std::string& expected) {
    std::size_t difference = supplied.size() ^ expected.size();
    for (std::size_t i = 0; i < expected.size(); ++i) difference |= static_cast<unsigned char>(expected[i]) ^ (i < supplied.size() ? static_cast<unsigned char>(supplied[i]) : 0U);
    return difference == 0;
}
void authenticate(const Request& request, const std::string& token) {
    const auto auth = request.headers.find("authorization");
    if (request.headers.count("origin") || auth == request.headers.end() || !equalToken(auth->second, "Bearer " + token)) throw Failure{401, "UNAUTHORIZED"};
}
void respond(SOCKET socket, int status, const std::string& body) {
    const std::string response = "HTTP/1.1 " + std::to_string(status) + " " + (status == 200 ? "OK" : "Error")
        + "\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(body.size())
        + "\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n\r\n" + body;
    const auto deadline = Clock::now() + std::chrono::seconds(1);
    std::size_t sent = 0;
    while (sent < response.size()) {
        waitReady(socket, true, deadline);
        const int count = send(socket, response.data() + sent, static_cast<int>(response.size() - sent), 0);
        if (count <= 0) return;
        sent += static_cast<std::size_t>(count);
    }
}
void log(std::ofstream& stream, const std::string& endpoint, int status, Clock::time_point start, const char* code) {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_s(&utc, &now);
    stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ") << ' ' << endpoint << ' ' << status << ' '
           << std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count() << "ms " << code << '\n';
    stream.flush();
}
void connection(SOCKET socket, const std::string& token, std::ofstream& logs) {
    const auto start = Clock::now();
    std::string endpoint = "other";
    int status = 200;
    const char* code = "OK";
    std::string body;
    try {
        auto request = headers(socket, start + requestDeadline);
        if (request.endpoint == "/health" || request.endpoint == "/evaluate") endpoint = request.endpoint;
        authenticate(request, token);
        if (request.method == "GET" && request.endpoint == "/health") {
            if (request.length != 0) invalid();
            body = "{\"ok\":true,\"service\":\"neo-cube\",\"version\":\"0.1.0\"}";
        } else if (request.method == "POST" && request.endpoint == "/evaluate") {
            if (request.length == 0 || !request.headers.count("content-type")) invalid();
            const auto type = lower(request.headers.at("content-type"));
            if (type != "application/json" && type != "application/json; charset=utf-8") invalid();
            while (request.body.size() < request.length) request.body += receive(socket, start + requestDeadline, request.length - request.body.size());
            body = evaluate(validate(request.body));
        } else throw Failure{404, "NOT_FOUND"};
    } catch (const Failure& error) { status = error.status; code = error.code; }
      catch (...) { status = 500; code = "INTERNAL_ERROR"; }
    if (status != 200) body = "{\"ok\":false,\"error\":{\"code\":\"" + std::string(code) + "\"}}";
    try { respond(socket, status, body); } catch (...) {}
    log(logs, endpoint, status, start, code);
}
} // namespace local_api

int main() {
    using namespace local_api;
    const char* rawToken = std::getenv("CHOISYS_LOCAL_API_TOKEN");
    const std::string token = rawToken ? rawToken : "";
    if (token.size() < 32 || token.size() > 256 || !std::all_of(token.begin(), token.end(), [](unsigned char c) { return c >= 33 && c <= 126; })) {
        std::cerr << "Local service authentication is not configured.\n";
        return 1;
    }
    std::ofstream logs;
    try {
        const char* configured = std::getenv("CHOISYS_LOCAL_LOG_DIR");
        const char* profile = std::getenv("USERPROFILE");
        if (!configured && !profile) throw std::runtime_error("missing log directory");
        const auto directory = configured ? std::filesystem::path(configured) : std::filesystem::path(profile) / "Documents" / "Scenarys" / "logs" / "neo-cube";
        std::filesystem::create_directories(directory);
        logs.open(directory / "service.log", std::ios::app);
        if (!logs) throw std::runtime_error("log unavailable");
    } catch (const std::exception& error) { std::cerr << "Local service logging is unavailable: " << error.what() << '\n'; return 1; }
    WSADATA winsock{};
    if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0) return 1;
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) { WSACleanup(); return 1; }
    const BOOL exclusive = TRUE;
    if (setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)) == SOCKET_ERROR) { closesocket(listener); WSACleanup(); return 1; }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR || listen(listener, 16) == SOCKET_ERROR) {
        std::cerr << "Cannot bind local service at 127.0.0.1:8765.\n";
        closesocket(listener); WSACleanup(); return 1;
    }
    std::cout << "neo-cube service ready at 127.0.0.1:8765\n";
    for (;;) {
        sockaddr_in peer{};
        int size = sizeof(peer);
        SOCKET client = accept(listener, reinterpret_cast<sockaddr*>(&peer), &size);
        if (client == INVALID_SOCKET) break;
        if (peer.sin_addr.s_addr == htonl(INADDR_LOOPBACK)) {
            // Non-blocking socket plus absolute deadlines bounds partial reads/writes.
            u_long nonblocking = 1;
            if (ioctlsocket(client, FIONBIO, &nonblocking) == 0) connection(client, token, logs);
        }
        shutdown(client, SD_BOTH);
        closesocket(client);
    }
    closesocket(listener);
    WSACleanup();
    return 1;
}
