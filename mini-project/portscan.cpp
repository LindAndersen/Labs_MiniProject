// Multi-threaded TCP port scanner with timeout.
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <optional>
#include <algorithm>
#include <chrono>

// Linux networking headers
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

// ---------- non-blocking connect with select() timeout ----------
bool is_port_open(const std::string& ip, int port, int timeout_ms) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return false;

    // Non-blocking
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0 || fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(s);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        close(s);
        return false;
    }

    int ret = connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret == 0) { // connected instantly
        close(s);
        return true;
    }
    if (errno != EINPROGRESS) { // failed immediately
        close(s);
        return false;
    }

    fd_set wfds, efds;
    FD_ZERO(&wfds); FD_ZERO(&efds);
    FD_SET(s, &wfds); FD_SET(s, &efds);

    timeval tv{};
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int sel = select(s + 1, nullptr, &wfds, &efds, &tv);
    if (sel <= 0) { // 0 timeout, -1 error
        close(s);
        return false;
    }

    int so_error = 0;
    socklen_t len = sizeof(so_error);
    if (getsockopt(s, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
        close(s);
        return false;
    }

    close(s);
    return (so_error == 0);
}

// ---------- argument parsing ----------
struct Args {
    std::string ip;
    int start_port = 1;
    int end_port   = 1024;
    int timeout_ms = 1000;
    int max_threads = 50;
};

void print_usage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " <ip> <port|start-end> [--timeout-ms=N] [--threads=M]\n\n"
              << "Examples:\n"
              << "  " << prog << " 127.0.0.1 80\n"
              << "  " << prog << " 127.0.0.1 1-1024 --timeout-ms=500 --threads=50\n";
}

std::optional<Args> parse_args(int argc, char** argv) {
    if (argc < 3) return std::nullopt;

    Args a;
    a.ip = argv[1];

    // parse port or range
    {
        std::string pr = argv[2];
        auto dash = pr.find('-');
        try {
            if (dash == std::string::npos) {
                a.start_port = a.end_port = std::stoi(pr);
            } else {
                a.start_port = std::stoi(pr.substr(0, dash));
                a.end_port   = std::stoi(pr.substr(dash + 1));
            }
        } catch (...) { return std::nullopt; }
    }

    if (a.start_port < 1 || a.end_port > 65535 || a.start_port > a.end_port) {
        return std::nullopt;
    }

    // optional flags
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        auto eq = arg.find('=');
        auto key = (eq == std::string::npos) ? arg : arg.substr(0, eq);
        auto val = (eq == std::string::npos) ? ""  : arg.substr(eq + 1);

        if (key == "--timeout-ms" && !val.empty()) {
            try { a.timeout_ms = std::max(1, std::stoi(val)); } catch (...) { return std::nullopt; }
        } else if (key == "--threads" && !val.empty()) {
            try { a.max_threads = std::max(1, std::stoi(val)); } catch (...) { return std::nullopt; }
        } else {
            return std::nullopt;
        }
    }
    return a;
}

std::mutex g_printMutex;

int main(int argc, char** argv) {
    auto argsOpt = parse_args(argc, argv);
    if (!argsOpt) {
        print_usage(argv[0]);
        return 1;
    }
    Args args = *argsOpt;

    // quick IPv4 format check
    in_addr tmp{};
    if (inet_pton(AF_INET, args.ip.c_str(), &tmp) != 1) {
        std::cerr << "Invalid IPv4 address: " << args.ip << "\n";
        return 1;
    }

    std::cout << "Scanning " << args.ip << " ports " << args.start_port << "-" << args.end_port
              << "  (timeout=" << args.timeout_ms << "ms, threads=" << args.max_threads << ")\n";

    std::vector<std::thread> workers;
    workers.reserve(args.max_threads);

    for (int port = args.start_port; port <= args.end_port; ++port) {
        // throttle: if we already have max_threads alive, join one
        if (workers.size() >= static_cast<size_t>(args.max_threads)) {
            workers.front().join();
            workers.erase(workers.begin());
        }

        workers.emplace_back([&, port]() {
            bool open = is_port_open(args.ip, port, args.timeout_ms);
            std::lock_guard<std::mutex> lock(g_printMutex);
            if (open) {
                std::cout << "[OPEN ] " << port << "\n";
            } else {
                // Uncomment to see closed/filtered:
                // std::cout << "[CLOSE] " << port << "\n";
            }
        });
    }

    for (auto& t : workers) t.join();
    std::cout << "Done.\n";
    return 0;
}
