#include <windows.h>
#include <iostream>
#include <fstream>
#include <array>

void handle_session(HANDLE pipe) {
    try {


        std::cout << "Client connected" << std::endl;

        
        uint64_t file_size = 0;
        DWORD bytes_read = 0;
        if (!ReadFile(pipe, &file_size, sizeof(file_size), &bytes_read, nullptr) || bytes_read != sizeof(file_size)) {
            throw std::runtime_error("Failed to read file size: ");
        }
        std::cout << "Expected file size: " << file_size << " bytes" << std::endl;

        
        std::ofstream out_file("received_file", std::ios::binary);
        if (!out_file) {
            throw std::runtime_error("Failed to open output file");
        }

        
        std::array<char, 4096> buffer;
        size_t total_received = 0;
        while (total_received < file_size) {
            DWORD bytes_to_read = static_cast<DWORD>(std::min<size_t>(buffer.size(), file_size - total_received));
            if (!ReadFile(pipe, buffer.data(), bytes_to_read, &bytes_read, nullptr)) {
                throw std::runtime_error("Failed to read file data: ");
            }
            if (bytes_read == 0) {
                std::cout << "Pipe closed by client" << std::endl;
                break;
            }

            out_file.write(buffer.data(), bytes_read);
            total_received += bytes_read;
        }

        if (total_received == file_size) {
            std::cout << "File received successfully" << std::endl;
        }
        else {
            std::cerr << "Incomplete file transfer: " << total_received << "/" << file_size << " bytes" << std::endl;
        }

        out_file.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in session: " << e.what() << std::endl;
    }
}

int main() {
    HANDLE pipe = INVALID_HANDLE_VALUE;
    try {
        const wchar_t* pipe_name = L"\\\\.\\pipe\\file_transfer_pipe";
        std::wcout << L"Server creating named pipe: " << pipe_name << std::endl;

        while (true) {
           
            pipe = CreateNamedPipeW(
                pipe_name,
                PIPE_ACCESS_INBOUND,
                PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                1, 
                4096, 
                4096, 
                0, 
                nullptr 
            );

            if (pipe == INVALID_HANDLE_VALUE) {
                throw std::runtime_error("Failed to create named pipe: ");
            }

            
            std::cout << "Waiting for client connection..." << std::endl;
            if (!ConnectNamedPipe(pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED) {
                CloseHandle(pipe);
                pipe = INVALID_HANDLE_VALUE;
                throw std::runtime_error("Failed to connect named pipe: " );
            }

            handle_session(pipe);

            
            DisconnectNamedPipe(pipe);
            CloseHandle(pipe);
            pipe = INVALID_HANDLE_VALUE;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in main: " << e.what() << std::endl;
        CloseHandle(pipe);       
        return 1;
    }
    return 0;
}