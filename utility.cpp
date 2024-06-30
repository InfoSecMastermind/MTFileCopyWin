#include <iostream>
#include <windows.h>
#include <vector>
#include <chrono>
#include <string>

// Link the kernel32.lib for using Windows API functions
#pragma comment(lib, "kernel32.lib")

// Structure to hold data for each thread
struct ThreadData {
    HANDLE sourceFile;  // Handle for the source file
    HANDLE destFile;    // Handle for the destination file
    DWORD64 offset;     // Offset in the file to start copying from
    DWORD chunkSize;    // Size of the chunk to be copied by this thread
    int threadNum;      // Number of the thread
    DWORD bytesCopied;  // Number of bytes actually copied by the thread
};

// Function executed by each thread to copy a chunk of the file
DWORD WINAPI CopyChunk(LPVOID param) {
    ThreadData* data = (ThreadData*)param;
    DWORD bytesRead, bytesWritten;
    std::vector<char> buffer(data->chunkSize);

    // Set the file pointer to the correct position
    LARGE_INTEGER li;
    li.QuadPart = data->offset;
    SetFilePointerEx(data->sourceFile, li, NULL, FILE_BEGIN);

    // Read the chunk from the source file
    if (ReadFile(data->sourceFile, buffer.data(), data->chunkSize, &bytesRead, NULL) && bytesRead > 0) {
        // Set the file pointer in the destination file
        li.QuadPart = data->offset;
        SetFilePointerEx(data->destFile, li, NULL, FILE_BEGIN);
        // Write the chunk to the destination file
        WriteFile(data->destFile, buffer.data(), bytesRead, &bytesWritten, NULL);
        // Store the number of bytes written
        data->bytesCopied = bytesWritten;
    } else {
        // If read fails, set bytesCopied to 0
        data->bytesCopied = 0;
    }

    return 0;
}

int main() {
    
    std::string sourcePath, destPath;
    int numThreads;



    std::cout << "Multithreaded File Copying Utility For Windows" << std::endl;

    // Ask for the source file path
    std::cout << "Enter the full source file path: ";
    std::getline(std::cin, sourcePath);

    // Ask for the destination file path
    std::cout << "Enter the full destination file path: ";
    std::getline(std::cin, destPath);

    // Ask for the number of threads
    std::cout << "Enter the number of threads: ";
    std::cin >> numThreads;

    // Open the source file for reading
    HANDLE sourceFile = CreateFileA(sourcePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (sourceFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening source file." << std::endl;
        return 1;
    }

    // Open the destination file for writing
    HANDLE destFile = CreateFileA(destPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (destFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error creating destination file." << std::endl;
        CloseHandle(sourceFile);
        return 1;
    }

    // Get the size of the source file
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(sourceFile, &fileSize)) {
        std::cerr << "Error getting file size." << std::endl;
        CloseHandle(sourceFile);
        CloseHandle(destFile);
        return 1;
    }

    // Print the file size
    std::cout << "File Size: " << fileSize.QuadPart << " bytes" << std::endl;

    // Calculate the size of each chunk to be copied by each thread
    DWORD chunkSize = (DWORD)((fileSize.QuadPart + numThreads - 1) / numThreads);
    std::vector<ThreadData> threadData(numThreads);
    std::vector<HANDLE> threads(numThreads);

    // Start the timer
    auto start = std::chrono::high_resolution_clock::now();

    // Create threads to copy chunks of the file
    for (int i = 0; i < numThreads; ++i) {
        threadData[i] = { sourceFile, destFile, i * chunkSize, chunkSize, i, 0 };
        threads[i] = CreateThread(NULL, 0, CopyChunk, &threadData[i], 0, NULL);
    }

    // Wait for all threads to finish
    WaitForMultipleObjects(numThreads, threads.data(), TRUE, INFINITE);

    // Stop the timer
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // Close the thread handles
    for (int i = 0; i < numThreads; ++i) {
        CloseHandle(threads[i]);
    }

    // Close the file handles
    CloseHandle(sourceFile);
    CloseHandle(destFile);

    // Print the details of the copy operation
    std::cout << std::endl << "Copy Details:" << std::endl;
    for (const auto& data : threadData) {
        std::cout << "Thread " << data.threadNum << ": " << data.bytesCopied << " bytes copied from offset " << data.offset << std::endl;
    }

    // Print the total time taken to copy the file
    std::cout << "Total Time Taken: " << elapsed.count() << " seconds" << std::endl;

    return 0;
}
