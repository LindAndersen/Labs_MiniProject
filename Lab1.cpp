#include <iostream>

void vulnerable_function(char* input) {
    char buffer[64];
    std::cin.getline(buffer, 256);
}

void win()
{
    std::cout << "You win!" << std::endl;
}

int main()
{
    std::cout << (void*)win << std::endl;
}

