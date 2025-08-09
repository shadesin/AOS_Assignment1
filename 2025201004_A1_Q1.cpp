#include <fcntl.h>       // open, O_*
#include <unistd.h>      // read, write, lseek, close, _exit
#include <sys/stat.h>    // mkdir
#include <errno.h>       // errno
#include <sys/mman.h>    // mmap, munmap
#include <sys/types.h>   // off_t

// Manual strlen without libc
int strLength(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

// Manual string to integer; returns -1 if invalid
int convertToInt(const char* str) {
    int val = 0; 
    int i = 0;
    if (str[0] == '-') return -1;
    while (str[i]) {
        if (str[i] < '0' || str[i] > '9') return -1;
        val = val * 10 + (str[i] - '0');
        i++;
    }
    return val;
}

// Write zero-terminated string to fd
void fdWriteStr(int fd, const char* str) {
    write(fd, str, strLength(str));
}

// Write integer to fd
void fdWriteInt(int fd, int num) {
    char buf[12];
    int i = 10; buf[11] = '\0';
    if (num == 0) { write(fd, "0", 1); return; }
    bool neg = false;
    if (num < 0) { neg = true; num = -num; }
    while (num > 0 && i >= 0) { buf[i--] = '0' + (num % 10); num /= 10; }
    if (neg && i >= 0) buf[i--] = '-';
    write(fd, buf + i + 1, 11 - i - 1);
}

void printUsage() {
    fdWriteStr(2, "Usage:\n");
    fdWriteStr(2, "./q1 <input_file> 0 <block_size>\n");
    fdWriteStr(2, "./q1 <input_file> 1\n");
    fdWriteStr(2, "./q1 <input_file> 2 <start_idx> <end_idx>\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage();
        _exit(1);
    }

    const char* inputFile = argv[1];
    int mode = convertToInt(argv[2]);

    // Validate mode & args
    if (mode == 0) {
        if (argc != 4) {
            fdWriteStr(2, "Error: Flag 0 requires block size.\n");
            printUsage();
            _exit(1);
        }
    } else if (mode == 1) {
        if (argc != 3) {
            fdWriteStr(2, "Error: Flag 1 takes no extra arguments.\n");
            printUsage();
            _exit(1);
        }
    } else {
        fdWriteStr(2, "Error: Only flag 0 and 1 implemented currently.\n");
        printUsage();
        _exit(1);
    }

    // For flag 0 parse block size
    int blockSize = 0;
    if (mode == 0) {
        blockSize = convertToInt(argv[3]);
        if (blockSize <= 0) {
            fdWriteStr(2, "Invalid block size.\n");
            _exit(1);
        }
        if (blockSize > 8 * 1024 * 1024) {
            fdWriteStr(2, "Warning: block size > 8MB may be inefficient.\n");
        }
    } else {
        // For flag 1 fixed chunk size
        blockSize = 1024 * 1024; // 1MB chunks for efficiency
    }

    // Create Assignment1 directory
    if (mkdir("Assignment1", 0700) == -1 && errno != EEXIST) {
        fdWriteStr(2, "Failed to create Assignment1 directory.\n");
        _exit(1);
    }

    // Open input file
    int fd_in = open(inputFile, O_RDONLY);
    if (fd_in == -1) {
        fdWriteStr(2, "Failed to open input file.\n");
        _exit(1);
    }

    // Build output filename: Assignment1/<mode>_<basename>
    const char* baseName = inputFile;
    for (int i = 0; inputFile[i] != '\0'; i++) {
        if (inputFile[i] == '/') baseName = inputFile + i + 1;
    }
    char outputPath[512];
    int pos = 0;
    const char* folder = "Assignment1/";
    for (int i = 0; folder[i] != '\0'; i++) outputPath[pos++] = folder[i];
    outputPath[pos++] = '0' + mode;
    outputPath[pos++] = '_';
    for (int i = 0; baseName[i] != '\0'; i++) outputPath[pos++] = baseName[i];
    outputPath[pos] = '\0';

    // Open output file
    int fd_out = open(outputPath, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd_out == -1) {
        fdWriteStr(2, "Failed to open output file.\n");
        close(fd_in);
        _exit(1);
    }

    // Get file size
    off_t fileSize = lseek(fd_in, 0, SEEK_END);
    if (fileSize == (off_t)-1) {
        fdWriteStr(2, "Failed to determine input file size.\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }

    // Reset input file pointer
    if (lseek(fd_in, 0, SEEK_SET) == (off_t)-1) {
        fdWriteStr(2, "Failed to reset input file pointer.\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }

    // Allocate buffer
    char* buffer = (char*) mmap(NULL, blockSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
        fdWriteStr(2, "Failed to allocate buffer memory.\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }

    if (mode == 0) {
        // ----- Block-wise reversal (flag 0) -----
        off_t totalBlocks = (fileSize + blockSize - 1) / blockSize;
        off_t processedBlocks = 0;

        while (processedBlocks < totalBlocks) {
            ssize_t currentBlockSize = blockSize;
            off_t bytesRemaining = fileSize - processedBlocks * blockSize;
            if (bytesRemaining < blockSize) currentBlockSize = bytesRemaining;

            ssize_t bytesRead = 0;
            while (bytesRead < currentBlockSize) {
                ssize_t r = read(fd_in, buffer + bytesRead, currentBlockSize - bytesRead);
                if (r == -1) {
                    fdWriteStr(2, "Error reading input file.\n");
                    munmap(buffer, blockSize);
                    close(fd_in);
                    close(fd_out);
                    _exit(1);
                }
                if (r == 0) break; // EOF
                bytesRead += r;
            }

            if (bytesRead == 0) break;

            // Reverse this block in place
            for (ssize_t i = 0; i < bytesRead / 2; i++) {
                char tmp = buffer[i];
                buffer[i] = buffer[bytesRead - i - 1];
                buffer[bytesRead - i - 1] = tmp;
            }

            ssize_t bytesWritten = 0;
            while (bytesWritten < bytesRead) {
                ssize_t w = write(fd_out, buffer + bytesWritten, bytesRead - bytesWritten);
                if (w == -1) {
                    fdWriteStr(2, "Error writing output file.\n");
                    munmap(buffer, blockSize);
                    close(fd_in);
                    close(fd_out);
                    _exit(1);
                }
                bytesWritten += w;
            }

            processedBlocks++;

            // Show progress
            write(1, "\rProgress: ", 11);
            fdWriteInt(1, (int)((processedBlocks * 100) / totalBlocks));
            write(1, "%", 1);
        }
    } else if (mode == 1) {
        // ----- Full file reversal (flag 1) -----
        off_t bytesProcessed = 0;
        off_t remaining = fileSize;

        while (remaining > 0) {
            ssize_t toRead = (remaining >= blockSize) ? blockSize : remaining;

            // Move file pointer to position before this chunk
            if (lseek(fd_in, remaining - toRead, SEEK_SET) == (off_t)-1) {
                fdWriteStr(2, "Seek error in input file.\n");
                munmap(buffer, blockSize);
                close(fd_in);
                close(fd_out);
                _exit(1);
            }

            ssize_t bytesRead = 0;
            while (bytesRead < toRead) {
                ssize_t r = read(fd_in, buffer + bytesRead, toRead - bytesRead);
                if (r < 0) {
                    fdWriteStr(2, "Read error.\n");
                    munmap(buffer, blockSize);
                    close(fd_in);
                    close(fd_out);
                    _exit(1);
                }
                if (r == 0) break;
                bytesRead += r;
            }

            if (bytesRead == 0) break;

            // Reverse chunk bytes
            for (ssize_t i = 0; i < bytesRead / 2; i++) {
                char tmp = buffer[i];
                buffer[i] = buffer[bytesRead - i - 1];
                buffer[bytesRead - i - 1] = tmp;
            }

            ssize_t bytesWritten = 0;
            while (bytesWritten < bytesRead) {
                ssize_t w = write(fd_out, buffer + bytesWritten, bytesRead - bytesWritten);
                if (w == -1) {
                    fdWriteStr(2, "Write error.\n");
                    munmap(buffer, blockSize);
                    close(fd_in);
                    close(fd_out);
                    _exit(1);
                }
                bytesWritten += w;
            }

            bytesProcessed += bytesRead;
            remaining -= bytesRead;

            // Show progress
            write(1, "\rProgress: ", 11);
            fdWriteInt(1, (int)((bytesProcessed * 100) / fileSize));
            write(1, "%", 1);
        }
    }

    write(1, "\n", 1);

    munmap(buffer, blockSize);
    close(fd_in);
    close(fd_out);
    return 0;
}
