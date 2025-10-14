#include <cstring>
#include <iostream>

void win()
{
    std::cout << "You win! Shell would spawn here." << std::endl;
    system("/bin/sh");
    exit(0);
}

void vulnerable()
{
    //bufffer overflow vulnerability
    char buf[64];

    std::cout << "Enter payload: ";

    // Vulnerable line: reading more than buffer size
    std::cin.getline(buf, 256);
    std::cout << "You entered: " << buf << std::endl;
}

int main()
{
    std::cout << "Address of win(): " << (void *)win << std::endl; // Leak for exploit
    vulnerable();
    return 0;
}
