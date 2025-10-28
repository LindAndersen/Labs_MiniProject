# C++ for Cybersecurity - Labs & Mini Project

This repository contains implementations for three hands-on labs and a mini-project as part of the Cybersecurity course at KMU, Department of Robotics Engineering.

## Table of Contents
- [Environment Setup](#environment-setup)
- [Lab 1: Buffer Overflow Exploitation](#lab-1-buffer-overflow-exploitation)
- [Lab 2: Reverse Engineering with Ghidra](#lab-2-reverse-engineering-with-ghidra)
- [Lab 3: Educational Keylogger (Windows)](#lab-3-educational-keylogger-windows)
- [Mini Project: Multi-Threaded Port Scanner](#mini-project-multi-threaded-port-scanner)

---

## Environment Setup

This project uses a Docker container with the necessary tools for cybersecurity labs.

### Prerequisites
- Docker installed on your system
- Basic knowledge of command-line operations

### Building the Docker Image

**MacOS:**
```sh
export DOCKER_DEFAULT_PLATFORM=linux/amd64
docker build -t ubuntu-cyber-machine .
```

**Windows:**
```sh
set DOCKER_DEFAULT_PLATFORM=linux/amd64
docker build -t ubuntu-cyber-machine .
```

### Running the Container

**MacOS:**
```sh
docker run -ti -v $(pwd):/home -w /home ubuntu-cyber-machine bash
```

**Windows:**
```sh
docker run -ti -v C:\Users\YourName\project:/home -w /home ubuntu-cyber-machine bash
```

This mounts your current directory inside the container at `/home`, allowing you to work with files on your host system.

---

## Lab 1: Buffer Overflow Exploitation

### Objective
Demonstrate how a classic stack-based buffer overflow can be exploited to hijack program control flow and redirect execution to a privileged function (`win()`).

### Files
- `lab1.cpp` - Vulnerable program with buffer overflow
- `payload.cpp` - Exploit payload generator
- `Makefile` - Build instructions

### Running Lab 1

#### 1. Compile the Vulnerable Program

```sh
make lab1
```

Or manually:
```sh
g++ -g -fno-stack-protector -no-pie -z execstack -o lab1 lab1.cpp
```

**Important:** These flags disable modern security protections and should NEVER be used in production code.

#### 2. Find the Offset

Run the program in GDB and use Metasploit pattern tools:

```sh
# Generate a unique pattern
/usr/share/metasploit-framework/tools/exploit/pattern_create.rb -l 100

# Run in GDB
gdb ./lab1
(gdb) run
# Paste the pattern when prompted

# After crash, find offset
/usr/share/metasploit-framework/tools/exploit/pattern_offset.rb -q <address>
```

#### 3. Craft and Deliver the Exploit

```sh
make payload
./payload | ./lab1
```

Or use the pattern file:
```sh
cat pattern.txt | ./lab1
```

### Expected Output
If successful, the program will jump to the `win()` function and print the flag message.

### Learning Outcomes
- Understanding memory layout and stack frames
- How unchecked input leads to control flow hijacking
- Importance of modern mitigations (stack canaries, ASLR, DEP)

---

## Lab 2: Reverse Engineering with Ghidra

### Objective
Analyze a stripped C++ binary to recover hidden logic, identify a password check, and extract an encrypted flag using static and dynamic analysis techniques.

### Files
- `crackme.cpp` - Source code (for reference only)
- `crackme` - Stripped binary (compiled with `-s -O2`)

### Running Lab 2

#### 1. Compile the Binary

```sh
make crackme
```

Or manually:
```sh
g++ -o crackme crackme.cpp -s -O2
```

#### 2. Static Analysis with Ghidra

1. Open Ghidra and create a new project
2. Import the `crackme` binary
3. Run the auto-analyzer (ensure "Demangle C++ Names" is enabled)
4. Navigate to the `main` function in the decompiler view
5. Look for:
   - String comparisons (`strcmp`, `operator==`)
   - Hardcoded strings (potential password)
   - Decryption routines (XOR operations)

#### 3. Find the Password

In the decompiled code, identify the hardcoded password string being compared to user input.

#### 4. Extract and Decrypt the Flag

Option A - Static Analysis:
1. Locate the encrypted byte array in the decompiler
2. Identify the XOR key (usually a small constant like `0x13`)
3. Write a Python script to decrypt:

```python
encrypted = [0x??, 0x??, ...]  # bytes from Ghidra
key = 0x13
flag = ''.join(chr(b ^ key) for b in encrypted)
print(flag)
```

Option B - Dynamic Analysis:
1. Run the binary and enter the correct password
2. Use a debugger to step through the decryption routine
3. Watch memory to see the flag appear in plaintext

### Expected Output
The correct password will decrypt and display a hidden flag.

### Learning Outcomes
- Mapping assembly to high-level logic
- Handling C++ constructs like `std::string`
- Defeating basic obfuscation techniques

---

## Lab 3: Educational Keylogger (Windows)

**Note:** This lab is platform-specific and will be completed by another team member.

---

## Mini Project: Multi-Threaded Port Scanner

### Objective
Create a TCP port scanner that checks whether ports on a target machine are open, using multiple threads for speed and a configurable timeout to avoid hanging on unresponsive ports.

### Files
- `portscan.cpp` - Multi-threaded port scanner implementation

### Features
- Multi-threaded scanning for improved performance
- Configurable timeout to prevent hanging
- Thread-safe output using mutex
- Support for custom IP addresses and port ranges

### Building the Port Scanner

```sh
make portscan
```

Or manually:

**Linux:**
```sh
g++ -o portscan portscan.cpp -pthread
```

**Windows:**
```sh
g++ -o portscan.exe portscan.cpp -lws2_32 -pthread
```

### Usage

```sh
./portscan <target_ip> <start_port> <end_port> [num_threads]
```

**Examples:**

```sh
# Scan localhost ports 1-1000 with 50 threads
./portscan 127.0.0.1 1 1000 50

# Scan common ports on a target
./portscan 192.168.1.1 20 100 20

# Quick scan of web-related ports
./portscan 127.0.0.1 80 443 10
```

### Parameters
- `target_ip` - IP address to scan (e.g., 127.0.0.1)
- `start_port` - Beginning of port range (1-65535)
- `end_port` - End of port range (1-65535)
- `num_threads` - (Optional) Number of concurrent threads (default: 50)

### Expected Output

```
Scanning 127.0.0.1 from port 1 to 1000 using 50 threads...
Port 22 is OPEN
Port 80 is OPEN
Port 443 is OPEN
...
Scan complete.
```

### Technical Implementation

The port scanner implements:
1. **Non-blocking Sockets:** Using `fcntl()` (Linux) or `ioctlsocket()` (Windows)
2. **Connection Timeout:** Using `select()` with a 1-second timeout
3. **Thread Pool:** Limiting concurrent threads to avoid resource exhaustion
4. **Thread Safety:** Using `std::mutex` for synchronized console output
5. **Error Handling:** Graceful handling of connection errors and timeouts

### Performance Considerations
- Default thread count (50) balances speed and system resources
- Timeout set to 1 second prevents hanging on filtered ports
- Thread pool prevents overwhelming the target or local system

### Ethical and Legal Notice

This tool is for **educational purposes only**. Port scanning can be considered:
- Network reconnaissance
- Potentially hostile activity by network administrators
- Illegal if performed without authorization

**Only scan:**
- Your own systems (127.0.0.1, localhost)
- Systems you own or have explicit written permission to test
- Lab environments specifically designed for security testing

Unauthorized port scanning may violate:
- Computer Fraud and Abuse Act (CFAA)
- Computer Misuse Act
- Local cybersecurity laws

### Learning Outcomes
- Low-level socket programming (POSIX sockets, Winsock2)
- Multi-threaded programming with `std::thread`
- Synchronization using `std::mutex`
- Non-blocking I/O and timeout handling
- Cross-platform C++ development (Linux/Windows)
- Ethical considerations in security tool development

---

## Quick Build Reference

Use the provided Makefile for easy compilation:

```sh
# Build Lab 1 vulnerable program
make lab1

# Build Lab 1 payload generator
make payload

# Build Lab 2 crackme binary
make crackme

# Build Mini Project port scanner
make portscan

# Build everything
make all

# Clean compiled binaries
make clean
```

---

## Project Structure

```
.
├── Dockerfile              # Docker environment configuration
├── Makefile               # Build automation
├── README.md              # This file
├── lab1.cpp               # Lab 1: Buffer overflow vulnerable program
├── payload.cpp            # Lab 1: Exploit payload generator
├── crackme.cpp            # Lab 2: Reverse engineering target
├── portscan.cpp           # Mini Project: Port scanner
├── pattern.txt            # Lab 1: Metasploit pattern for offset finding
└── STR.txt                # Lab 1: Steps to reproduce
```

---

## Deliverables

For complete submission requirements, see the assignment PDF. Each lab and the mini project require:

### Labs 1-2:
- Lab report (PDF) with explanations, screenshots, and reflective answers
- Working source code
- Documentation of methodology

### Mini Project:
- Well-commented source code (`portscan.cpp`)
- Build instructions (provided in this README)
- Screenshots showing successful execution
- One-page project report covering design decisions, security considerations, testing methodology, and lessons learned

---

## Troubleshooting

### Docker Issues
- **Platform mismatch:** Ensure `DOCKER_DEFAULT_PLATFORM=linux/amd64` is set
- **Volume mounting:** Verify your path is correct in the `-v` flag
- **Permission errors:** On Linux, you may need to run with `sudo`

### Compilation Issues
- **Missing dependencies:** Install build-essential (Linux) or MinGW (Windows)
- **Thread support:** Ensure `-pthread` flag is used for multi-threaded programs
- **Winsock errors:** On Windows, link with `-lws2_32`

### Port Scanner Issues
- **Permission denied:** Some systems require root/admin privileges for certain ports
- **Connection refused:** Normal for closed ports
- **Timeout:** Increase timeout if scanning over slow networks
- **Too many open files:** Reduce the number of threads

---

## References

- [Metasploit Pattern Tools](https://github.com/rapid7/metasploit-framework/tree/master/tools/exploit)
- [Ghidra Documentation](https://ghidra-sre.org/)
- [POSIX Sockets Guide](https://beej.us/guide/bgnet/)
- [Winsock2 Documentation](https://docs.microsoft.com/en-us/windows/win32/winsock/)

---
