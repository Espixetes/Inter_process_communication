#include <windows.h>
#include <iostream>
#include <fstream>
#include <array>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
        return 1;
    }

    HANDLE pipe = INVALID_HANDLE_VALUE;
    try {
        
        std::ifstream in_file(argv[1], std::ios::binary | std::ios::ate);
        if (!in_file) {
            throw std::runtime_error("Failed to open input file: " + std::string(argv[1]));
        }

        
        uint64_t file_size = in_file.tellg();
        in_file.seekg(0, std::ios::beg);

        
        const wchar_t* pipe_name = L"\\\\.\\pipe\\file_transfer_pipe";
        std::wcout << L"Connecting to named pipe: " << pipe_name << std::endl;

        pipe = CreateFileW(
            pipe_name,
            GENERIC_WRITE,
            0, 
            nullptr, 
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (pipe == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to connect to named pipe: ");
        }

        std::cout << "Connected to server" << std::endl;

       
        DWORD bytes_written = 0;
        if (!WriteFile(pipe, &file_size, sizeof(file_size), &bytes_written, nullptr) || bytes_written != sizeof(file_size)) {
            throw std::runtime_error("Failed to write file size: ");
        }
        std::cout << "Sending file of size: " << file_size << " bytes" << std::endl;

        
        std::array<char, 4096> buffer;
        size_t total_sent = 0;
        while (total_sent < file_size) {
            in_file.read(buffer.data(), buffer.size());
            size_t bytes_read = in_file.gcount();
            if (bytes_read == 0) {
                break;
            }

            if (!WriteFile(pipe, buffer.data(), static_cast<DWORD>(bytes_read), &bytes_written, nullptr) || bytes_written != bytes_read) {
                throw std::runtime_error("Failed to write file data: ");
            }

            total_sent += bytes_written;
        }

        in_file.close();
        CloseHandle(pipe);
        std::cout << "File sent successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        CloseHandle(pipe);
    }
    return 0;
}