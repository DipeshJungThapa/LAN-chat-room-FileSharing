# LAN Chat Room with File Transfer (Windows Version)

A terminal-based chat application for Windows that allows multiple clients to communicate over a LAN and transfer files.

## Prerequisites
- Windows OS
- MinGW-w64 installed with g++ compiler
- Basic network connectivity between machines

## Building

### Server
```bash
cd server
mingw32-make -f Makefile.win