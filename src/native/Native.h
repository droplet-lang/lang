/*
 * ============================================================
 *  Droplet 
 * ============================================================
 *  Copyright (c) 2025 Droplet Contributors
 *  All rights reserved.
 *
 *  Licensed under the MIT License.
 *  See LICENSE file in the project root for full license.
 *
 *  File: Native
 *  Created: 11/9/2025
 * ============================================================
 */
#ifndef DROPLET_NATIVE_H
#define DROPLET_NATIVE_H
#include "../vm/VM.h"
#include <string>

struct ITCP {
    virtual ~ITCP() = default;

    virtual bool connect(const std::string& host, int port) = 0;
    virtual bool send(const std::string& data) = 0;
    virtual std::string receive(int len) = 0;
    virtual void close() = 0;
};

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
struct TCP_Win32 : ITCP {
    SOCKET sock = INVALID_SOCKET;

    TCP_Win32() {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
    }

    ~TCP_Win32() { close(); WSACleanup(); }

    bool connect(const std::string& host, int port) override {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        return ::connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0;
    }

    bool send(const std::string& data) override {
        return ::send(sock, data.c_str(), data.size(), 0) >= 0;
    }

    std::string receive(int len) override {
        std::string buffer(len, 0);
        int received = ::recv(sock, &buffer[0], len, 0);
        if (received <= 0) return "";
        buffer.resize(received);
        return buffer;
    }

    void close() override {
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
    }
};
#else
// POSIX / Android / Linux / macOS
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

struct TCP_POSIX : ITCP {
    int sock = -1;

    ~TCP_POSIX() { close(); }

    bool connect(const std::string& host, int port) override {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        return ::connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0;
    }

    bool send(const std::string& data) override {
        return ::send(sock, data.c_str(), data.size(), 0) >= 0;
    }

    std::string receive(int len) override {
        std::string buffer(len, 0);
        int received = ::recv(sock, &buffer[0], len, 0);
        if (received <= 0) return "";
        buffer.resize(received);
        return buffer;
    }

    void close() override {
        if (sock >= 0) {
            ::close(sock);
            sock = -1;
        }
    }
};
#endif

// PLATFORM INDEPENDENT NATIVE FUNCTIONS
void native_len(VM& vm, uint8_t argc);
void native_print(VM& vm, uint8_t argc);
void native_println(VM& vm, uint8_t argc);
void native_str(VM& vm, uint8_t argc);
void native_input(VM& vm, uint8_t argc);
void native_int(VM& vm, uint8_t argc);
void native_float(VM& vm, uint8_t argc);
void native_exit(VM& vm, uint8_t argc);
void native_append(VM& vm, uint8_t argc);
void native_forEach_simple(VM& vm, uint8_t argc);
void native_tcp_create(VM& vm, uint8_t argc);
void native_tcp_connect(VM& vm, uint8_t argc);
void native_tcp_send(VM& vm, uint8_t argc);
void native_tcp_receive(VM& vm, uint8_t argc);
void native_tcp_close(VM& vm, uint8_t argc);



// made inline just to shut up compiler warning, no special case
inline void register_native_functions(VM& vm) {
    vm.register_native("exit", native_exit);
    vm.register_native("print", native_print);
    vm.register_native("println", native_println);
    vm.register_native("str", native_str);
    vm.register_native("len", native_len);
    vm.register_native("input", native_input);
    vm.register_native("append", native_append);
    vm.register_native("forEach", native_forEach_simple);
    vm.register_native("tcp_create", native_tcp_create);
    vm.register_native("tcp_connect", native_tcp_connect);
    vm.register_native("tcp_send", native_tcp_send);
    vm.register_native("tcp_receive", native_tcp_receive);
    vm.register_native("tcp_close", native_tcp_close);


}

#endif //DROPLET_NATIVE_H