#include <cstring>
#include <iostream>
using namespace std;

void win() {
    cout << "You win! Shell would spawn here." << endl;
    system("/bin/sh");        // Linux
    // WinExec("calc.exe", 0);   // Windows
}

void vulnerable() {
    char buf[64];
    cout << "Enter payload: ";
    cin.getline(buf, 256); // Reads 256 bytes into 64-byte buffer → overflow!
    cout << "You entered: " << buf << endl;
}

int main() {
    cout << "Address of win(): " << (void*)win << endl; // Leak for exploit
    win();
    //vulnerable();
    return 0;
}

