#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <windows.h>
#include "../shared/constants.h"

#pragma comment(lib, "ws2_32.lib")

struct ThreadData {
    SOCKET client_socket;
};

DWORD WINAPI receive_messages(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    SOCKET client_socket = data->client_socket;
    char buffer[BUFFER_SIZE];
    
    while (true) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            std::cout << "Disconnected from server" << std::endl;
            ExitProcess(0);
        }
        buffer[bytes_received] = '\0';  // Null-terminate the string
        std::cout << buffer << std::endl;
    }
    return 0;
}

void send_file(SOCKET client_socket, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }

    std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);
    int file_size = (int)file.tellg();
    file.seekg(0, std::ios::beg);

    char header[BUFFER_SIZE];
    int message_type = FILE_TRANSFER;
    int filename_length = (int)filename.size();

    memcpy(header, &message_type, sizeof(int));
    memcpy(header + sizeof(int), &filename_length, sizeof(int));
    memcpy(header + sizeof(int) * 2, filename.c_str(), filename_length);
    memcpy(header + sizeof(int) * 2 + filename_length, &file_size, sizeof(int));

    int header_size = sizeof(int) * 3 + filename_length;
    send(client_socket, header, header_size, 0);

    char ack[5];
    recv(client_socket, ack, 5, 0);

    char buffer[FILE_BUFFER_SIZE];
    while (!file.eof()) {
        file.read(buffer, FILE_BUFFER_SIZE);
        int bytes_read = (int)file.gcount();
        if (bytes_read > 0) {
            send(client_socket, buffer, bytes_read, 0);
        }
    }

    file.close();
    std::cout << "File sent: " << filename << " (" << file_size << " bytes)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        return 1;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Windows-specific IP address conversion
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid address" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    // Get username from user
    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
    
    // Send username to server
    char message[BUFFER_SIZE];
    int message_type = USERNAME_SET;
    int username_length = (int)username.size();
    memcpy(message, &message_type, sizeof(int));
    memcpy(message + sizeof(int), &username_length, sizeof(int));
    memcpy(message + sizeof(int) * 2, username.c_str(), username_length);
    send(client_socket, message, sizeof(int) * 2 + username_length, 0);

    std::cout << "Connected to server as '" << username << "'. Type your messages or /sendfile <filename> to send a file." << std::endl;

    // Create thread data
    ThreadData thread_data;
    thread_data.client_socket = client_socket;

    // Create thread for receiving messages
    HANDLE thread_handle = CreateThread(
        NULL,                   // Default security attributes
        0,                      // Default stack size
        receive_messages,       // Thread function
        &thread_data,          // Parameter to thread function
        0,                      // Default creation flags
        NULL);                  // Don't need thread identifier

    if (thread_handle == NULL) {
        std::cerr << "Error creating thread" << std::endl;
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::string input;
    while (true) {
        std::getline(std::cin, input);

        if (input.substr(0, 9) == "/sendfile") {
            if (input.length() > 10) {
                std::string filepath = input.substr(10);
                send_file(client_socket, filepath);
            } else {
                std::cout << "Usage: /sendfile <filename>" << std::endl;
            }
        } else {
            char message[BUFFER_SIZE];
            int message_type = MESSAGE;
            int message_length = (int)input.size();
            
            if (message_length > 0) {
                memcpy(message, &message_type, sizeof(int));
                memcpy(message + sizeof(int), input.c_str(), message_length);
                send(client_socket, message, sizeof(int) + message_length, 0);
            }
        }
    }

    CloseHandle(thread_handle);
    closesocket(client_socket);
    WSACleanup();
    return 0;
}