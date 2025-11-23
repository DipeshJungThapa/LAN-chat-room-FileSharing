#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>
#include <csignal>

// Platform-specific includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#include "../shared/constants.h"

// Cross-platform socket initialization
class SocketInitializer {
public:
    SocketInitializer() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
#endif
    }
    
    ~SocketInitializer() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

// Client information structure
struct ClientInfo {
    SOCKET socket;
    std::string username;
    std::string ip_address;
    std::chrono::system_clock::time_point connected_time;
    std::atomic<bool> active{true};
    
    ClientInfo(SOCKET s, const std::string& name, const std::string& ip) 
        : socket(s), username(name), ip_address(ip), 
          connected_time(std::chrono::system_clock::now()) {}
};

// Global state
std::vector<std::shared_ptr<ClientInfo>> clients;
std::mutex clients_mutex;
std::atomic<bool> server_running{true};

// Utility function to get error message
std::string get_socket_error() {
#ifdef _WIN32
    return std::to_string(WSAGetLastError());
#else
    return std::string(strerror(errno));
#endif
}

// Broadcast message to all clients except sender
void broadcast(SOCKET sender, const std::string& message, const std::string& sender_username) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::string formatted_message = "[" + sender_username + "]: " + message;
    
    for (auto& client : clients) {
        if (client->socket != sender && client->active) {
            ssize_t sent = send(client->socket, formatted_message.c_str(), 
                               formatted_message.size(), 0);
            if (sent <= 0) {
                client->active = false;
            }
        }
    }
}

// Broadcast system notification to all clients
void broadcast_notification(const std::string& notification) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::string formatted = "*** " + notification + " ***";
    
    for (auto& client : clients) {
        if (client->active) {
            ssize_t sent = send(client->socket, formatted.c_str(), 
                               formatted.size(), 0);
            if (sent <= 0) {
                client->active = false;
            }
        }
    }
}

// Handle file transfer
bool handle_file_transfer(SOCKET client_socket, const std::string& filename, 
                         int64_t file_size, const std::string& username) {
    // Create uploads directory if it doesn't exist
#ifdef _WIN32
    system("if not exist uploads mkdir uploads");
#else
    system("mkdir -p uploads");
#endif
    
    std::string filepath = "uploads/" + filename;
    std::ofstream file(filepath, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Failed to create file: " << filepath << std::endl;
        return false;
    }

    char buffer[FILE_BUFFER_SIZE];
    int64_t total_bytes_received = 0;
    
    auto start_time = std::chrono::steady_clock::now();
    
    while (total_bytes_received < file_size) {
        int bytes_to_receive = std::min(static_cast<int64_t>(FILE_BUFFER_SIZE), 
                                       file_size - total_bytes_received);
        ssize_t bytes_received = recv(client_socket, buffer, bytes_to_receive, 0);
        
        if (bytes_received <= 0) {
            std::cerr << "Error receiving file data" << std::endl;
            file.close();
            return false;
        }
        
        file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;
    }
    
    file.close();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double speed = (total_bytes_received / 1024.0 / 1024.0) / (duration.count() / 1000.0);
    
    std::cout << "✓ File received from " << username << ": " << filename 
              << " (" << total_bytes_received << " bytes, " 
              << speed << " MB/s)" << std::endl;
    
    return true;
}

// Get client IP address
std::string get_client_ip(SOCKET socket) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    if (getpeername(socket, (struct sockaddr*)&addr, &addr_len) == 0) {
        return std::string(inet_ntoa(addr.sin_addr));
    }
    return "unknown";
}

// Handle individual client
void handle_client(SOCKET client_socket) {
    std::string username = "Unknown";
    std::string client_ip = get_client_ip(client_socket);
    std::shared_ptr<ClientInfo> client_info;
    
    char buffer[BUFFER_SIZE];
    
    try {
        // First, receive username
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            throw std::runtime_error("Failed to receive username");
        }
        
        int32_t message_type;
        std::memcpy(&message_type, buffer, sizeof(int32_t));
        
        if (message_type == USERNAME_SET) {
            int32_t username_length;
            std::memcpy(&username_length, buffer + sizeof(int32_t), sizeof(int32_t));
            
            if (username_length > 0 && username_length <= MAX_USERNAME_LENGTH) {
                username = std::string(buffer + sizeof(int32_t) * 2, username_length);
                
                // Create and add client to list
                client_info = std::make_shared<ClientInfo>(client_socket, username, client_ip);
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    clients.push_back(client_info);
                }
                
                std::cout << "✓ New client connected: " << username 
                         << " (" << client_ip << ")" << std::endl;
                
                // Notify all other clients
                broadcast_notification(username + " joined the chat");
            } else {
                throw std::runtime_error("Invalid username length");
            }
        } else {
            throw std::runtime_error("Expected USERNAME_SET message");
        }

        // Main message loop
        while (server_running && client_info->active) {
            bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            
            if (bytes_received <= 0) {
                break;
            }

            std::memcpy(&message_type, buffer, sizeof(int32_t));

            if (message_type == MESSAGE) {
                std::string message(buffer + sizeof(int32_t), 
                                  bytes_received - sizeof(int32_t));
                std::cout << "[" << username << "]: " << message << std::endl;
                broadcast(client_socket, message, username);
            } 
            else if (message_type == FILE_TRANSFER) {
                int32_t filename_length;
                std::memcpy(&filename_length, buffer + sizeof(int32_t), sizeof(int32_t));
                
                std::string filename(buffer + sizeof(int32_t) * 2, filename_length);

                int64_t file_size;
                std::memcpy(&file_size, buffer + sizeof(int32_t) * 2 + filename_length, 
                           sizeof(int64_t));

                // Send acknowledgment
                std::string ack = "READY";
                send(client_socket, ack.c_str(), ack.size(), 0);
                
                // Handle file transfer
                if (handle_file_transfer(client_socket, filename, file_size, username)) {
                    broadcast_notification(username + " shared file: " + filename);
                }
            }
            else if (message_type == DISCONNECT) {
                std::cout << "Client " << username << " disconnecting gracefully" << std::endl;
                break;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error handling client " << username << ": " << e.what() << std::endl;
    }

    // Cleanup
    if (client_info) {
        client_info->active = false;
        
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [client_socket](const std::shared_ptr<ClientInfo>& c) {
                    return c->socket == client_socket;
                }),
            clients.end()
        );
        
        std::cout << "✗ Client disconnected: " << username 
                 << " (" << client_ip << ")" << std::endl;
        
        broadcast_notification(username + " left the chat");
    }
    
    closesocket(client_socket);
}

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "\n✗ Server shutting down..." << std::endl;
    server_running = false;
}

int main() {
    std::cout << "╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  LAN Chat Room Server v2.0                     ║" << std::endl;
    std::cout << "║  Cross-platform Edition                        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    // Setup signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        // Initialize sockets
        SocketInitializer socket_init;
        
        // Create server socket
        SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed: " + get_socket_error());
        }

        // Set socket options
        int opt = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, 
                      (char*)&opt, sizeof(opt)) < 0) {
            std::cerr << "Warning: Failed to set SO_REUSEADDR" << std::endl;
        }

        // Bind socket
        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_socket, (struct sockaddr*)&server_addr, 
                sizeof(server_addr)) == SOCKET_ERROR) {
            throw std::runtime_error("Bind failed: " + get_socket_error());
        }

        // Listen for connections
        if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
            throw std::runtime_error("Listen failed: " + get_socket_error());
        }

        std::cout << "✓ Server started successfully" << std::endl;
        std::cout << "✓ Listening on port " << PORT << std::endl;
        std::cout << "✓ Max clients: " << MAX_CLIENTS << std::endl;
        std::cout << "✓ Press Ctrl+C to stop the server" << std::endl;
        std::cout << "\n" << std::string(50, '=') << std::endl;

        // Set socket to non-blocking mode on Linux
#ifndef _WIN32
        int flags = fcntl(server_socket, F_GETFL, 0);
        fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
#endif

        // Accept client connections
        while (server_running) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            SOCKET client_socket = accept(server_socket, 
                                         (struct sockaddr*)&client_addr, 
                                         &client_len);
            
            if (client_socket == INVALID_SOCKET) {
#ifndef _WIN32
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    // No pending connections, sleep briefly
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
#endif
                if (server_running) {
                    std::cerr << "Accept failed: " << get_socket_error() << std::endl;
                }
                continue;
            }

            // Handle client in a new thread
            std::thread client_thread(handle_client, client_socket);
            client_thread.detach();
        }

        // Cleanup
        closesocket(server_socket);
        
        // Disconnect all clients
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (auto& client : clients) {
                closesocket(client->socket);
            }
            clients.clear();
        }
        
        std::cout << "✓ Server stopped" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
