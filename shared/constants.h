#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

// Network configuration
constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 4096;          // Increased for better performance
constexpr int MAX_CLIENTS = 100;           // Support more clients
constexpr int FILE_BUFFER_SIZE = 65536;    // 64KB for faster file transfers
constexpr int MAX_USERNAME_LENGTH = 32;
constexpr int MAX_MESSAGE_LENGTH = 2048;

// Message types
enum MessageType : int32_t {
    MESSAGE = 1,
    FILE_TRANSFER = 2,
    USERNAME_SET = 3,
    DISCONNECT = 4,
    PING = 5,
    CLIENT_LIST = 6
};

// Protocol constants
constexpr int PROTOCOL_VERSION = 2;

#endif