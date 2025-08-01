#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <windows.h>
#include "../shared/constants.h"

#pragma comment(lib, "ws2_32.lib")

struct ClientInfo {
    SOCKET socket;
    std::string username;
    
    ClientInfo(SOCKET s, const std::string& name) : socket(s), username(name) {}
};

std::vector<ClientInfo> clients;
CRITICAL_SECTION clients_mutex;

void broadcast(SOCKET sender, const std::string& message, const std::string& sender_username) {
    EnterCriticalSection(&clients_mutex);
    std::string formatted_message = sender_username + ": " + message;
    for (size_t i = 0; i < clients.size(); i++) {
        ClientInfo& client = clients[i];
        if (client.socket != sender) {
            send(client.socket, formatted_message.c_str(), (int)formatted_message.size(), 0);
        }
    }
    LeaveCriticalSection(&clients_mutex);
}

void broadcast_notification(const std::string& notification) {
    EnterCriticalSection(&clients_mutex);
    for (size_t i = 0; i < clients.size(); i++) {
        ClientInfo& client = clients[i];
        send(client.socket, notification.c_str(), (int)notification.size(), 0);
    }
    LeaveCriticalSection(&clients_mutex);
}

void handle_file_transfer(SOCKET client_socket, const std::string& filename, int file_size) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return;
    }

    char buffer[FILE_BUFFER_SIZE];
    int bytes_received;
    int total_bytes_received = 0;

    while (total_bytes_received < file_size) {
        bytes_received = recv(client_socket, buffer, FILE_BUFFER_SIZE, 0);
        if (bytes_received <= 0) break;
        file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;
    }

    file.close();
    std::cout << "File received: " << filename << " (" << total_bytes_received << " bytes)" << std::endl;
}

struct ThreadData {
    SOCKET client_socket;
};

DWORD WINAPI handle_client(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    SOCKET client_socket = data->client_socket;
    std::string username = "Unknown";
    
    char buffer[BUFFER_SIZE];
    
    // First, receive username
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        delete data;
        return 0;
    }
    
    int message_type;
    memcpy(&message_type, buffer, sizeof(int));
    
    if (message_type == USERNAME_SET) {
        int username_length;
        memcpy(&username_length, buffer + sizeof(int), sizeof(int));
        username = std::string(buffer + sizeof(int) * 2, username_length);
        
        // Add client to list
        EnterCriticalSection(&clients_mutex);
        clients.push_back(ClientInfo(client_socket, username));
        LeaveCriticalSection(&clients_mutex);
        
        std::cout << "New client connected: " << username << std::endl;
        
        // Notify all other clients
        std::string notification = "*** " + username + " joined the chat ***";
        broadcast_notification(notification);
    }

    // Main message loop
    while (true) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }

        memcpy(&message_type, buffer, sizeof(int));

        if (message_type == MESSAGE) {
            std::string message(buffer + sizeof(int), bytes_received - sizeof(int));
            std::cout << username << ": " << message << std::endl;
            broadcast(client_socket, message, username);
        } 
        else if (message_type == FILE_TRANSFER) {
            int filename_length;
            memcpy(&filename_length, buffer + sizeof(int), sizeof(int));
            std::string filename(buffer + sizeof(int) * 2, filename_length);

            int file_size;
            memcpy(&file_size, buffer + sizeof(int) * 2 + filename_length, sizeof(int));

            std::string ack = "READY";
            send(client_socket, ack.c_str(), (int)ack.size(), 0);
            handle_file_transfer(client_socket, filename, file_size);
            
            // Notify all clients about file transfer
            std::string notification = "*** " + username + " shared a file: " + filename + " ***";
            broadcast_notification(notification);
        }
    }

    // Remove client from list and notify others
    EnterCriticalSection(&clients_mutex);
    for (size_t i = 0; i < clients.size(); i++) {
        if (clients[i].socket == client_socket) {
            clients.erase(clients.begin() + i);
            break;
        }
    }
    LeaveCriticalSection(&clients_mutex);
    
    std::cout << username << " disconnected" << std::endl;
    
    // Notify all other clients
    std::string notification = "*** " + username + " left the chat ***";
    broadcast_notification(notification);
    
    closesocket(client_socket);
    delete data;
    return 0;
}

int main() {
    // Initialize critical section
    InitializeCriticalSection(&clients_mutex);
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        DeleteCriticalSection(&clients_mutex);
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        DeleteCriticalSection(&clients_mutex);
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        DeleteCriticalSection(&clients_mutex);
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        DeleteCriticalSection(&clients_mutex);
        return 1;
    }

    std::cout << "Server started. Listening on port " << PORT << std::endl;

    while (true) {
        sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        std::cout << "New client connected" << std::endl;
        
        // Create thread data
        ThreadData* thread_data = new ThreadData;
        thread_data->client_socket = client_socket;
        
        // Create thread for handling client
        HANDLE thread_handle = CreateThread(
            NULL,                   // Default security attributes
            0,                      // Default stack size
            handle_client,          // Thread function
            thread_data,            // Parameter to thread function
            0,                      // Default creation flags
            NULL);                  // Don't need thread identifier

        if (thread_handle == NULL) {
            std::cerr << "Error creating thread" << std::endl;
            closesocket(client_socket);
            delete thread_data;
            continue;
        }
        
        // Close thread handle (thread continues to run)
        CloseHandle(thread_handle);
    }

    closesocket(server_socket);
    WSACleanup();
    DeleteCriticalSection(&clients_mutex);
    return 0;
}