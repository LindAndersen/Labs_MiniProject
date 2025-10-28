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

## Lab 2: Reverse Engineering a C++ Binary with Ghidra

### Objective
Analyze a stripped C++ binary to recover hidden logic, identify a password check, and extract an encrypted flag using static and dynamic analysis techniques.

### Why This Matters
Malware and proprietary software often hide secrets (passwords, keys, C2 addresses) using simple obfuscation like XOR. Reverse engineering is essential for malware analysis, vulnerability research, and digital forensics.

### Files
- `crackme.cpp` - Source code for the crackme challenge
- `crackme` - Stripped binary (compiled with `-s -O2`)
- `xor.py` - Python decryption script
- `screenshots/` - Documentation of the reverse engineering process

### The Challenge

The crackme binary implements a password-protected program that:
- Prompts for a password
- Compares it to a hardcoded value
- If correct, decrypts and prints a flag using XOR encryption with a constant key
- Clears sensitive data from memory after use

### Running Lab 2

#### 1. Compile the Binary

Compile with symbols stripped and optimizations enabled to mimic real-world binaries:

```sh
cd lab2
g++ -o crackme crackme.cpp -s -O2
```

**Flags explanation:**
- `-s` : Strips debugging symbols, making reverse engineering harder
- `-O2` : Enables optimizations, changing code structure

#### 2. Static Analysis in Ghidra

Open Ghidra and create a new project, then import your binary and run the auto-analyzer.

![Ghidra Analysis Options](lab2/screenshots/Screenshot%202025-10-28%20at%2020.04.24.png)

**Crucially, ensure "Demangle C++ Names" is enabled**—this converts mangled symbols like `_ZN6Crackme4mainEv` back to readable function names.

#### 3. Navigate to Main Function

1. In the Symbol Tree, find the `main` function (or `entry` if main is not visible)
2. Double-click to view in the Decompiler window
3. The decompiled C++ code will show the program logic

![Ghidra Decompiler View](lab2/screenshots/Screenshot%202025-10-28%20at%2020.13.17.png)

#### 4. Identify Key Components

Navigate to the main function in the decompiler view and look for:
- **String comparisons** (`strcmp`, `operator==`)
- **Hardcoded strings** (potential password)
- **Function calls** that might handle decryption

![Ghidra Full Analysis View](lab2/screenshots/Screenshot%202025-10-28%20at%2020.17.12.png)

In the decompiled code, you should identify:
- **Password**: `secret123`
- **XOR Key**: `this_is_a_constant_key`
- **Encrypted Data**: Array of hexadecimal bytes

**Tip**: Use the Decompiler's "Find" feature (Search > For Strings) to search for text like `memcmp` or password comparison logic.

![Decompiler Find Feature](lab2/screenshots/Screenshot%202025-10-28%20at%2020.35.28.png)

#### 5. Locate the Encrypted Flag

Find the function responsible for flag decryption. In the decompiler, identify:
- The encrypted byte array
- The XOR key (in this case, a string constant rather than a single byte)
- Note the sequence of encrypted bytes

From the decompiled code, locate the byte array initialization:

```cpp
local_b0[0x10] = 0x72;
local_b0[0x11] = 0x65;
local_b0[0x12] = 0x74;
local_b0[0x13] = 0x7d;
// ... more bytes
```

The encrypted form extracted from analysis:
```python
encrypted = [0x32, 0x24, 0x28, 0x34, 0x24, 0x1d, 0x1b, 0x36,
             0x12, 0x00, 0x0a, 0x1c, 0x31, 0x00, 0x11, 0x02,
             0x1c, 0x11, 0x2b, 0x16]
```

#### 6. Decrypt the Flag

**Option A: Static Analysis (Python Script)**

Write a short Python script to XOR each byte with the key and convert the result to ASCII:

```python
encrypted = [0x32, 0x24, 0x28, 0x34, 0x24, 0x1d, 0x1b, 0x36,
             0x12, 0x00, 0x0a, 0x1c, 0x31, 0x00, 0x11, 0x02,
             0x1c, 0x11, 0x2b, 0x16]
key = "this_is_a_constant_key"

flag = ""
for i in range(len(encrypted)):
    flag += chr(encrypted[i] ^ ord(key[i % len(key)]))

print(flag)
```

Run the decryption script:
```sh
python3 xor.py
# Output: FLAG{this_is_secret}
```

### Solution Methods

**Method 1: Use the Correct Password**
```sh
./crackme
# Enter password: secret123
# Output: FLAG{this_is_secret}
```

**Method 2: Extract and Decrypt (Reverse Engineering)**
```sh
python3 xor.py
# Output: FLAG{this_is_secret}
```

### Key Findings

From Ghidra analysis:
- **Hardcoded Password**: `secret123` - easily found in string references
- **XOR Encryption Key**: `this_is_a_constant_key` - visible in decompiled code
- **Encryption Algorithm**: Simple XOR cipher (reversible operation)
- **Security Weakness**: All secrets are stored in plaintext within the binary

### Expected Output
```
FLAG{this_is_secret}
```

---

## Lab 3: Educational Keylogger (Windows)

### Objective
Build a Windows keylogger that demonstrates low-level input monitoring using global hooks—while incorporating ethical safeguards to prevent misuse.

### Why This Matters
Keyloggers are common in both offensive toolkits and legitimate monitoring software (e.g., parental controls). Understanding their internals helps defenders detect them and developers build responsibly.

### Files
- `keylogger.cpp` - Educational keylogger implementation
- `ENABLE_LOGGING` - Required file to activate logging
- `keylog.txt` - Output log file
- `documentation/` - Technical documentation and diagrams

### Implementation Overview

#### Design with Safety First

The keylogger includes built-in ethical safeguards:
- **Does not log anything by default** - requires `ENABLE_LOGGING` file to be present
- **Allows clean exit** - Press ESC to exit at any time
- **Logs to file** - Outputs to `keylog.txt` for transparency
- **Thread-safe operation** - Uses mutex to prevent race conditions

#### Technical Architecture

**Windows API Components:**

1. **Global Keyboard Hook** - Uses `SetWindowsHookEx(WH_KEYBOARD_LL, ...)` for low-level keyboard monitoring
2. **Message Loop** - Keeps the hook alive in `main()` function
3. **Hook Callback** - `LowLevelKeyboardProc` receives every keystroke system-wide

**Key Concepts:**

![Main Functionality](lab3/documentation/keylogger.cpp-func-darc.png)

The keylogger separates concerns:
- **Main thread** - Administrates the hook (setup, monitoring, cleanup)
- **Hook callback** - Processes keystrokes independently when input is received

![Hook Function Flow](lab3/documentation/Hook-func.png)

The hook operates as part of the system's hook chain:
```
Input Controller → HookA → Our Hook → HookB → System → Application
```

#### Character Encoding Handling

The implementation properly handles Unicode character encoding:
- **Virtual Keys** - Logical keyboard meanings (e.g., `VK_TAB`)
- **Scan Codes** - Physical key positions
- **UTF-16 to UTF-8 Conversion** - Uses `ToUnicodeEx()` and `WideCharToMultiByte()`

This ensures proper logging of international characters and special keys.

### Running Lab 3

#### 1. Compile the Keylogger

```sh
cd lab3
g++ -o keylogger.exe keylogger.cpp -luser32
```

**Required library:**
- `-luser32` - Links Windows User32.dll for hook functions

#### 2. Enable Logging

Create the safety file:
```sh
# Windows (Command Prompt)
type nul > ENABLE_LOGGING

# Windows (PowerShell)
New-Item -ItemType File -Name ENABLE_LOGGING
```

#### 3. Run the Keylogger

```sh
./keylogger.exe
```

**Expected console output:**
```
Keylogger started. Logging only when "ENABLE_LOGGING" exists in the current directory.
Press ESC to stop.
```

#### 4. Test the Keylogger

- Type some text in any application
- Press special keys (Enter, Tab, Arrow keys, etc.)
- Check `keylog.txt` for logged output
- Press ESC to exit cleanly

#### 5. Review the Log

```sh
cat keylog.txt
```

Example output:
```
[KEYLOGGER_STARTED]
Hello[SPACE]World[ENTER]
Test[TAB]123[ESC]
[KEYLOGGER_STOPPED]
```

### Key Implementation Details

#### Keystroke Processing

The `vk_to_string()` function converts virtual key codes to readable strings:
- **Special keys** - `[SPACE]`, `[ENTER]`, `[BACKSPACE]`, `[TAB]`, `[ESC]`
- **Function keys** - `[F1]` through `[F24]`
- **Regular characters** - Converted using keyboard layout and state

#### Thread Safety

Uses `std::mutex` to ensure safe file writing:
```cpp
std::lock_guard<std::mutex> lock(log_mutex);
std::ofstream ofs(LOG_FILENAME, std::ios::app | std::ios::binary);
```

#### Graceful Cleanup

When ESC is pressed:
1. Hook signals main thread with `WM_QUIT`
2. `hook_active` flag set to `false`
3. `UnhookWindowsHookEx()` removes hook from chain
4. File handles closed and resources cleaned up

### Legal & Ethical Disclaimer

**This lab is for educational purposes only.** Unauthorized monitoring of user input violates privacy laws (e.g., CFAA, GDPR, Computer Misuse Act). You must have **explicit written permission** to run such tools on any system other than your own.

**Only test on:**
- Your own machine
- Systems you own
- Lab environments with explicit permission

### Learning Outcomes
- Hands-on experience with Windows internals and Win32 API
- Understanding low-level keyboard hooks (`WH_KEYBOARD_LL`)
- Thread safety with mutexes in system callbacks
- Ethical coding practices and safeguards
- Character encoding (UTF-16 to UTF-8 conversion)
- Defensive awareness of common surveillance techniques
- Understanding how hook chains work in Windows

### Technical Documentation

For detailed technical information about Windows API, hook mechanisms, and character encoding, see `lab3/documentation/README.md`.

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
