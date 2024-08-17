#include <cstring>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

bool patch(std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern, const std::vector<uint8_t>& new_bytes);
size_t scan_pattern(const std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern);

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    // read file into memory
    std::ifstream file(argv[1], std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file " << argv[1] << std::endl;
        return 1;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::vector<uint8_t> data(size);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();
     
    // patterns to search for and replace
    std::vector<uint8_t> replace_list[][2] = { 
        { 
            { 0x85, 0xc0, 0x74, 0x07, 0x6a, 0x01, 0xe8, 0xcd, 0x71, 0x00, 0x00, 0x83 }, 
            { 0x31, 0xc0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x83 },
        },
        { 
            { 0x74, 0x11, 0x6a, 0x01, 0xc7, 0x05, 0x40, 0xa6, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0xe8, 0x12, 0x66, 0x00, 0x00, },
            { 0xeb, 0x11, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, },
        },
        {
            { 0xff, 0xd7, 0x85, 0xc0, 0x0f, 0x85, 0xaf, 0x01, 0x00, 0x00, 0x56, 0xe8 },
            { 0xff, 0xd7, 0x85, 0xc0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x56, 0xe8 },
        },
        {
            { 0x00, 0x00, 0xff, 0x15, 0x0c, 0x40, 0x01, 0x10, 0x85, 0xc0, 0x0f, 0x85, 0xa1, 0x00, 0x00 },
            { 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xe9, 0xa1, 0x00, 0x00 },
        },
    };

    // apply patches
    int i = 1;
    for (auto& replace : replace_list) {
        if (!patch(data, replace[0], replace[1])) {
            std::cerr << "Error: could not find pattern " << i << std::endl;
            for (auto& byte : replace[0]) {
                std::cerr << std::hex << std::setw(2) << std::setfill('0') <<  static_cast<int>(byte) << " ";
            }
            std::cerr << std::endl;
            return 1;
        }
        i++;
    }

    // write patched data to file
    std::string out_filename = std::string(argv[1]) + ".patched.dll";
    std::ofstream out(out_filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: could not open file " << out_filename << " for writing." << std::endl;
        return 1;
    }

    out.write(reinterpret_cast<char*>(data.data()), size);
    std::cout << "Patched file written to " << out_filename << std::endl;
    return 0;
}

bool patch(std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern, const std::vector<uint8_t>& new_bytes)
{
    size_t offset = scan_pattern(data, pattern);
    if (offset == data.size()) {
        return false;
    }
    memcpy(data.data() + offset, new_bytes.data(), new_bytes.size());
    return true;
}

size_t scan_pattern(const std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern)
{
    bool found = false;
    size_t offset = 0;
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i] == pattern[0]) {
            found = true;
            for (size_t j = 1; j < pattern.size(); j++) {
                if (data[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                offset = i;
                break;
            }
        }
    }

    return offset;
}