#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <algorithm>

const std::string password = "secret123";
const std::string key = "this_is_a_constant_key"; // Key used for XOR encryption
const size_t BUFFER_SIZE = 64; // Max input length to prevent buffer overflow

int main(int argc, char** argv) {
    std::string out;
    std::string flag = "FLAG{this_is_secret}";
    const size_t flag_len = flag.size();
    std::vector<unsigned char> encrypted(flag_len);
    char buffer[BUFFER_SIZE];

    for (size_t i = 0; i < flag_len; ++i) {
        // XOR each character of the flag with the key (repeating the key as necessary)
        encrypted[i] = static_cast<unsigned char>(flag[i]) ^ static_cast<unsigned char>(key[i % key.size()]);
    }

    // Clear sensitive data from memory
    std::fill(flag.begin(), flag.end(), '\0');

    std::cout << "Enter password: ";
    // Get user input safely using sizeof(buffer) as the limit
    std::cin.getline(buffer, sizeof(buffer));

    // Check if the input operation failed (e.g., due to exceeding buffer size)
    if (std::cin.fail()) {
        std::cout << "Input failed or line was too long for the buffer." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return 1;
    }

    if (password == buffer) {
        // Resize out to hold the decrypted flag
        out.resize(flag_len);
        for (size_t i = 0; i < flag_len; ++i) {
            // Decrypt the flag by XORing again with the same key
            out[i] = static_cast<char>(encrypted[i] ^ static_cast<unsigned char>(key[i % key.size()]));
        }
        std::cout << out << std::endl;

        // Clear sensitive data from memory
        std::fill(out.begin(), out.end(), '\0');
    } else {
        std::cout << "Access denied." << std::endl;
    }

    return 0;
}
