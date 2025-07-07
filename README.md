# LAN Chat Room with File Transfer (Windows Version)

A terminal-based chat application for Windows that allows multiple clients to communicate over a Local Area Network (LAN) and transfer files seamlessly.

## Project Members
- **Aryam Ghimire**
- **Dipesh Thapa**

## About the Project

This is a multi-client chat room application built in C++ that enables real-time communication between multiple users connected to the same network. The application features a client-server architecture where one machine acts as the server hosting the chat room, and other machines connect as clients to participate in the conversation.

### Key Features
- **Multi-client support**: Multiple users can join the same chat room simultaneously
- **Real-time messaging**: Messages are instantly broadcast to all connected clients
- **Username system**: Each user sets a unique username when joining
- **File sharing**: Users can send files to the server and notify other participants
- **Join/leave notifications**: Automatic notifications when users join or leave the chat
- **Windows-optimized**: Built specifically for Windows using MinGW compiler

## Project Structure

```
LAN-chat-room-FileSharing/
├── client/
│   ├── client.cpp          # Client application source code
│   ├── client.exe          # Compiled client executable
│   └── Makefile.win        # Build configuration for client
├── server/
│   ├── server.cpp          # Server application source code
│   ├── server.exe          # Compiled server executable
│   └── Makefile.win        # Build configuration for server
├── shared/
│   └── constants.h         # Shared constants and configuration
└── README.md              # This file
```

## File Responsibilities

### Client Files
- **`client.cpp`**: Main client application that handles:
  - Connecting to the server
  - Username setup and registration
  - Sending and receiving messages
  - File transfer functionality
  - User interface for chat interaction

- **`client.exe`**: Compiled executable that users run to join the chat room

### Server Files
- **`server.cpp`**: Server application that manages:
  - Accepting multiple client connections
  - Broadcasting messages between clients
  - Handling file transfers and storage
  - Managing user sessions and usernames
  - Coordinating chat room activities

- **`server.exe`**: Compiled server executable that hosts the chat room

### Shared Files
- **`constants.h`**: Configuration file containing:
  - Server port number (default: 8080)
  - Buffer sizes for messages and files
  - Message type definitions
  - Maximum client limits

### Build Files
- **`Makefile.win`**: Build configuration files for compiling the applications using MinGW

## Prerequisites

Before running the application, ensure you have:

- **Windows Operating System**
- **MinGW-w64** installed with g++ compiler
- **Basic network connectivity** between machines on the same LAN
- **Firewall permissions** for the applications (if prompted)

## Building the Application

### 1. Build the Server
```bash
cd server
mingw32-make -f Makefile.win
```

### 2. Build the Client
```bash
cd client
mingw32-make -f Makefile.win
```

**Note**: You may see harmless warnings about `#pragma comment` - these can be ignored as the compilation will succeed.

## Running the Application

### Step 1: Start the Server
On the machine that will host the chat room:
```bash
cd server
./server.exe
```

You should see:
```
Server started. Listening on port 8080
```

### Step 2: Connect Clients
On each machine that wants to join the chat:
```bash
cd client
./client.exe <server_ip_address>
```

**Examples:**
- Same machine: `./client.exe 127.0.0.1`
- Different machine: `./client.exe 192.168.1.100`

### Step 3: Set Username
When prompted, enter your desired username:
```
Enter your username: Alice
Connected to server as 'Alice'. Type your messages or /sendfile <filename> to send a file.
```

## Using the Chat Room

### Basic Messaging
Simply type your message and press Enter:
```
Hello everyone!
```

Other users will see:
```
Alice: Hello everyone!
```

### File Sharing
To send a file, use the `/sendfile` command:
```
/sendfile C:\path\to\your\file.txt
```

Other users will be notified:
```
*** Alice shared a file: file.txt ***
```

### System Notifications
The chat room automatically shows when users join or leave:
```
*** Bob joined the chat ***
*** Alice left the chat ***
```

## How It Works

1. **Server Setup**: The server creates a socket and listens for incoming connections on port 8080
2. **Client Connection**: Clients connect to the server using the server's IP address
3. **Username Registration**: Each client sends their chosen username to the server
4. **Message Broadcasting**: When a client sends a message, the server broadcasts it to all other connected clients
5. **File Transfer**: Files are sent to the server and stored locally, with notifications sent to all clients
6. **Thread Management**: The server uses Windows threads to handle multiple clients simultaneously

## Network Configuration

### Finding Your IP Address
To find your machine's IP address for others to connect:
```bash
ipconfig
```
Look for the "IPv4 Address" under your active network adapter.

### Port Configuration
The default port is 8080. To change it:
1. Edit `PORT` value in `shared/constants.h`
2. Recompile both server and client
3. Ensure the port is not blocked by firewall

## Troubleshooting

### Common Issues

**"Connection failed"**
- Check if server is running
- Verify IP address is correct
- Ensure firewall allows the connection

**"Bind failed"**
- Port 8080 might be in use
- Try changing the port in `constants.h`

**"WSAStartup failed"**
- Windows networking issue
- Restart the application
- Check network adapter settings

### File Transfer Issues
- Use full file paths: `C:\Users\YourName\Documents\file.txt`
- Ensure file exists and is readable
- Check available disk space on server machine

## Technical Details

- **Language**: C++11
- **Compiler**: MinGW-w64 GCC
- **Networking**: Windows Sockets (Winsock2)
- **Threading**: Windows API (CreateThread)
- **Architecture**: Client-Server model
- **Protocol**: TCP/IP

## Limitations

- Windows-only (uses Windows-specific APIs)
- Files are stored on server machine only
- No message history or persistence
- No encryption (messages sent in plain text)
- Limited to local network connections

## Future Enhancements

Potential improvements for the project:
- Cross-platform compatibility (Linux/macOS)
- Message encryption for security
- File broadcasting to all clients
- Chat history and logging
- User authentication system
- Graphical user interface (GUI)
- Private messaging between users

## License

This project is created for educational purposes as part of a networking course assignment.

---

**Created by**: Aryam Ghimire and Dipesh Thapa  
**Course**: Computer Networks  
**Date**: 2025