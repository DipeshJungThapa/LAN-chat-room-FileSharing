# LAN Chat Room with File Transfer

A terminal-based chat application that allows multiple clients to communicate over a Local Area Network (LAN) and transfer files seamlessly. Now with cross-platform support!

## Project Members
- **Aryam Ghimire**
- **Dipesh Thapa**

## About the Project

This is a multi-client chat room application built in C++ that enables real-time communication between multiple users connected to the same network. The application features a client-server architecture where one machine acts as the server hosting the chat room, and other machines connect as clients to participate in the conversation.

### Key Features
- **Multi-client support**: Up to 100 users can join the chat room simultaneously
- **Real-time messaging**: Messages are instantly broadcast to all connected clients
- **Username system**: Each user sets a unique username when joining
- **File sharing**: Users can send files to the server and notify other participants
- **Join/leave notifications**: Automatic notifications when users join or leave the chat
- **Cross-platform**: Works on Linux, macOS, and Windows

## Prerequisites

### Linux
```bash
sudo apt install build-essential g++ make
```

### macOS
```bash
xcode-select --install
```

### Windows
- **MinGW-w64** or **MSYS2** with g++ compiler

## Building the Application

### Linux/macOS
```bash
# Build server
cd server
make

# Build client
cd client
make
```

### Windows
```bash
# Build server
cd server
mingw32-make

# Build client
cd client
mingw32-make
```

## Running the Application

### Step 1: Start the Server
On the machine that will host the chat room:
```bash
cd server
./server
```

You should see:
```
Server started. Listening on port 8080
```

### Step 2: Connect Clients
On each machine that wants to join the chat:
```bash
cd client
./client <server_ip_address>
```

**Examples:**
- Same machine: `./client 127.0.0.1`
- Different machine: `./client 192.168.1.100`

### Step 3: Set Username
When prompted, enter your desired username:
```
Enter your username: Alice
```

You will see:
```
✓ Logged in as 'Alice'

╔════════════════════════════════════════════════╗
║              Available Commands                ║
╠════════════════════════════════════════════════╣
║ /help               - Show this help message   ║
║ /sendfile <path>    - Send a file to server    ║
║ /quit or /exit      - Disconnect from server   ║
║ Any other text      - Send as chat message     ║
╚════════════════════════════════════════════════╝

> 
```

## Using the Chat Room

### Basic Messaging
Simply type your message and press Enter:
```
> Hello everyone!
```

Other users will see:
```
14:25:30 [Alice]: Hello everyone!
```

### Available Commands

**Get Help:**
```
> /help
```

**Send a File:**
```
> /sendfile /path/to/file.txt
> /sendfile ~/Documents/photo.jpg
> /sendfile C:\Users\Alice\file.pdf
```

**Disconnect:**
```
> /quit
```

### File Sharing
When you send a file, you'll see progress:
```
Uploading file.txt (1024000 bytes)...
Progress: 100% (1024000/1024000 bytes)
✓ File sent: file.txt (1024000 bytes, 45.2 MB/s)
```

All users will be notified:
```
*** Alice shared file: file.txt ***
```

Files are saved in the `server/uploads/` directory.

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
6. **Thread Management**: The server uses C++ threads to handle multiple clients simultaneously

## Network Configuration

### Finding Your IP Address

**Linux:**
```bash
hostname -I
```

**macOS:**
```bash
ipconfig getifaddr en0
```

**Windows:**
```bash
ipconfig
```

Look for the IPv4 address (usually starts with 192.168.x.x for local networks).

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
- Try changing the port in `shared/constants.h`

**File Transfer Issues**
- Use full file paths
- Ensure file exists and is readable
- Check available disk space on server machine

## Technical Details
- **Language**: C++17
- **Compiler**: GCC/Clang/MinGW
- **Networking**: BSD Sockets / Winsock2
- **Threading**: C++ std::thread
- **Architecture**: Client-Server model
- **Protocol**: TCP/IP

---

**Created by**: Aryam Ghimire and Dipesh Thapa  
**Course**: Network Programming  
**Date**: 2025
