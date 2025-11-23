#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <csignal>
#include <iomanip>

// Platform-specific includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <conio.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <termios.h>
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

// Global state
std::atomic<bool> client_running{true};
std::atomic<bool> connected{false};
SOCKET client_socket = INVALID_SOCKET;

// Utility function to get error message
std::string get_socket_error() {
#ifdef _WIN32
    return std::to_string(WSAGetLastError());
#else
    return std::string(strerror(errno));
#endif
}

// Get current timestamp
std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    return ss.str();
}

// Receive messages from server
void receive_messages(SOCKET socket) {
    char buffer[BUFFER_SIZE];
    
    while (client_running && connected) {
        ssize_t bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            if (client_running) {
                std::cout << "\n✗ Disconnected from server" << std::endl;
                connected = false;
                client_running = false;
            }
            break;
        }
        
        buffer[bytes_received] = '\0';
        std::cout << "\r" << get_timestamp() << " " << buffer << std::endl;
        std::cout << "> " << std::flush;
    }
}

// Send file to server
bool send_file(SOCKET socket, const std::string& filepath, const std::string& username) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cerr << "✗ Failed to open file: " << filepath << std::endl;
        return false;
    }

    // Extract filename from path
    std::string filename = filepath;
    size_t last_slash = filepath.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        filename = filepath.substr(last_slash + 1);
    }
    
    int64_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Prepare header
    char header[BUFFER_SIZE];
    int32_t message_type = FILE_TRANSFER;
    int32_t filename_length = filename.size();

    std::memcpy(header, &message_type, sizeof(int32_t));
    std::memcpy(header + sizeof(int32_t), &filename_length, sizeof(int32_t));
    std::memcpy(header + sizeof(int32_t) * 2, filename.c_str(), filename_length);
    std::memcpy(header + sizeof(int32_t) * 2 + filename_length, &file_size, sizeof(int64_t));

    int header_size = sizeof(int32_t) * 2 + filename_length + sizeof(int64_t);
    
    // Send header
    if (send(socket, header, header_size, 0) <= 0) {
        std::cerr << "✗ Failed to send file header" << std::endl;
        file.close();
        return false;
    }

    // Wait for acknowledgment
    char ack[6];
    if (recv(socket, ack, 5, 0) <= 0) {
        std::cerr << "✗ Failed to receive acknowledgment" << std::endl;
        file.close();
        return false;
    }

    // Send file data
    char buffer[FILE_BUFFER_SIZE];
    int64_t total_sent = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    std::cout << "Uploading " << filename << " (" << file_size << " bytes)..." << std::endl;
    
    while (!file.eof()) {
        file.read(buffer, FILE_BUFFER_SIZE);
        std::streamsize bytes_read = file.gcount();
        
        if (bytes_read > 0) {
            ssize_t sent = send(socket, buffer, bytes_read, 0);
            if (sent <= 0) {
                std::cerr << "✗ Failed to send file data" << std::endl;
                file.close();
                return false;
            }
            total_sent += sent;
            
            // Show progress
            int progress = (total_sent * 100) / file_size;
            std::cout << "\rProgress: " << progress << "% (" 
                     << total_sent << "/" << file_size << " bytes)" << std::flush;
        }
    }

    file.close();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double speed = (total_sent / 1024.0 / 1024.0) / (duration.count() / 1000.0);
    
    std::cout << "\n✓ File sent: " << filename << " (" << total_sent << " bytes, " 
             << speed << " MB/s)" << std::endl;
    
    return true;
}

// Send text message to server
bool send_message(SOCKET socket, const std::string& text) {
    char message[BUFFER_SIZE];
    int32_t message_type = MESSAGE;
    int32_t message_length = text.size();
    
    if (message_length > MAX_MESSAGE_LENGTH) {
        std::cerr << "✗ Message too long (max " << MAX_MESSAGE_LENGTH << " characters)" << std::endl;
        return false;
    }
    
    std::memcpy(message, &message_type, sizeof(int32_t));
    std::memcpy(message + sizeof(int32_t), text.c_str(), message_length);
    
    ssize_t sent = send(socket, message, sizeof(int32_t) + message_length, 0);
    return sent > 0;
}

// Display help information
void show_help() {
    std::cout << "\n╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              Available Commands                ║" << std::endl;
    std::cout << "╠════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ /help               - Show this help message   ║" << std::endl;
    std::cout << "║ /sendfile <path>    - Send a file to server    ║" << std::endl;
    std::cout << "║ /quit or /exit      - Disconnect from server   ║" << std::endl;
    std::cout << "║ Any other text      - Send as chat message     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
}

// Signal handler
void signal_handler(int signal) {
    std::cout << "\n✗ Disconnecting..." << std::endl;
    client_running = false;
    connected = false;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 192.168.1.100" << std::endl;
        return 1;
    }

    std::cout << "╔════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  LAN Chat Room Client v2.0                     ║" << std::endl;
    std::cout << "║  Cross-platform Edition                        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Setup signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        // Initialize sockets
        SocketInitializer socket_init;

        // Create client socket
        client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client_socket == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed: " + get_socket_error());
        }

        // Setup server address
        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        
        // Convert IP address
        if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
            throw std::runtime_error("Invalid server IP address");
        }

        std::cout << "Connecting to server " << argv[1] << ":" << PORT << "..." << std::endl;

        // Connect to server
        if (connect(client_socket, (struct sockaddr*)&server_addr, 
                   sizeof(server_addr)) == SOCKET_ERROR) {
            throw std::runtime_error("Connection failed: " + get_socket_error());
        }

        std::cout << "✓ Connected to server" << std::endl;
        connected = true;

        // Get username from user
        std::string username;
        std::cout << "\nEnter your username (max " << MAX_USERNAME_LENGTH << " characters): ";
        std::getline(std::cin, username);
        
        // Trim and validate username
        if (username.empty() || username.length() > MAX_USERNAME_LENGTH) {
            throw std::runtime_error("Invalid username");
        }
        
        // Send username to server
        char message[BUFFER_SIZE];
        int32_t message_type = USERNAME_SET;
        int32_t username_length = username.size();
        
        std::memcpy(message, &message_type, sizeof(int32_t));
        std::memcpy(message + sizeof(int32_t), &username_length, sizeof(int32_t));
        std::memcpy(message + sizeof(int32_t) * 2, username.c_str(), username_length);
        
        if (send(client_socket, message, sizeof(int32_t) * 2 + username_length, 0) <= 0) {
            throw std::runtime_error("Failed to send username");
        }

        std::cout << "\n✓ Logged in as '" << username << "'" << std::endl;
        show_help();

        // Start receiver thread
        std::thread receiver_thread(receive_messages, client_socket);

        // Main input loop
        std::string input;
        while (client_running && connected) {
            std::cout << "> " << std::flush;
            
            if (!std::getline(std::cin, input)) {
                break;
            }
            
            if (input.empty()) {
                continue;
            }

            // Handle commands
            if (input == "/quit" || input == "/exit") {
                std::cout << "Disconnecting..." << std::endl;
                break;
            }
            else if (input == "/help") {
                show_help();
            }
            else if (input.substr(0, 9) == "/sendfile") {
                if (input.length() > 10) {
                    std::string filepath = input.substr(10);
                    // Remove leading/trailing whitespace
                    size_t start = filepath.find_first_not_of(" \t");
                    size_t end = filepath.find_last_not_of(" \t");
                    if (start != std::string::npos && end != std::string::npos) {
                        filepath = filepath.substr(start, end - start + 1);
                        send_file(client_socket, filepath, username);
                    }
                } else {
                    std::cout << "Usage: /sendfile <path/to/file>" << std::endl;
                }
            }
            else {
                // Send as regular message
                if (!send_message(client_socket, input)) {
                    std::cerr << "✗ Failed to send message" << std::endl;
                    break;
                }
            }
        }

        // Send disconnect message
        int32_t disconnect_type = DISCONNECT;
        send(client_socket, (char*)&disconnect_type, sizeof(int32_t), 0);

        // Cleanup
        client_running = false;
        connected = false;
        
        if (receiver_thread.joinable()) {
            receiver_thread.join();
        }
        
        closesocket(client_socket);
        
        std::cout << "\n✓ Disconnected from server" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        
        if (client_socket != INVALID_SOCKET) {
            closesocket(client_socket);
        }
        
        return 1;
    }
}
