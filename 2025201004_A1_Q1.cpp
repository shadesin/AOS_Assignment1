#include <fcntl.h>       // open, O_*
#include <unistd.h>      // read, write, lseek, close, _exit
#include <sys/stat.h>    // mkdir
#include <errno.h>       // errno
#include <sys/mman.h>    // mmap, munmap
#include <sys/types.h>   // off_t

// Compute string length without libc
int strLength(const char* str) {
    int length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

// Convert string to positive integer; return -1 if invalid
int convertToInt(const char* str) {
    int result = 0, i = 0;
    if (str[0] == '-') return -1;
    while (str[i]) {
        if (str[i] < '0' || str[i] > '9') return -1;
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return result;
}

// Write zero-terminated string to fd
void fdWriteStr(int fd, const char* str) {
    write(fd, str, strLength(str));
}

// Write integer to fd as decimal digits
void fdWriteInt(int fd, int number) {
    char buffer[12];
    int index = 10;
    buffer[11] = '\0';
    if (number == 0) {
        write(fd, "0", 1);
        return;
    }
    bool negative = false;
    if (number < 0) {
        negative = true;
        number = -number;
    }
    while (number > 0 && index >= 0) {
        buffer[index--] = '0' + (number % 10);
        number /= 10;
    }
    if (negative && index >= 0) {
        buffer[index--] = '-';
    }
    write(fd, buffer + index + 1, 10 - index);
}

void printInstructions() {
    fdWriteStr(2, "Usage:\n");
    fdWriteStr(2, "./q1 <input_file> 0 <block_size>\n");
    fdWriteStr(2, "./q1 <input_file> 1\n");
    fdWriteStr(2, "./q1 <input_file> 2 <start_index> <end_index>\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printInstructions();
        _exit(1);
    }

    const char* inputFile = argv[1];
    int mode = convertToInt(argv[2]);

    if (mode != 0) {
        fdWriteStr(2, "Error: This version supports only mode 0 (block-wise reversal).\n");
        printInstructions();
        _exit(1);
    }

    if (argc != 4) {
        fdWriteStr(2, "Error: Flag 0 requires a block size argument.\n");
        printInstructions();
        _exit(1);
    }

    int blockSize = convertToInt(argv[3]);
    if (blockSize <= 0) {
        fdWriteStr(2, "Error: Block size must be a positive integer.\n");
        _exit(1);
    }
    if (blockSize > 8 * 1024 * 1024) {
        fdWriteStr(2, "Warning: Block size exceeds 8MB, which might be inefficient.\n");
    }

    // Create output directory
    if (mkdir("Assignment1", 0700) == -1 && errno != EEXIST) {
        fdWriteStr(2, "Failed to create directory 'Assignment1'.\n");
        _exit(1);
    }

    // Open input file
    int fd_in = open(inputFile, O_RDONLY);
    if (fd_in == -1) {
        fdWriteStr(2, "Could not open input file.\n");
        _exit(1);
    }

    // Open output file
    int fd_out = open("Assignment1/output_file", O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd_out == -1) {
        fdWriteStr(2, "Could not open or create output file.\n");
        close(fd_in);
        _exit(1);
    }

    // Determine input file size
    off_t fileSize = lseek(fd_in, 0, SEEK_END);
    if (fileSize < 0) {
        fdWriteStr(2, "Failed to determine input file size.\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }
    if (lseek(fd_in, 0, SEEK_SET) == (off_t)-1) {
        fdWriteStr(2, "Failed to reset file position to start.\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }

    // Allocate buffer using mmap (pure syscall allocation)
    char* buffer = (char*) mmap(NULL, blockSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
        fdWriteStr(2, "Failed to allocate memory buffer.\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }

    off_t totalBlocks = (fileSize + blockSize - 1) / blockSize;
    off_t blocksProcessed = 0;

    while (blocksProcessed < totalBlocks) {
        ssize_t currentBlockSize = blockSize;
        off_t remaining = fileSize - blocksProcessed * blockSize;
        if (remaining < blockSize) {
            currentBlockSize = remaining;
        }

        ssize_t totalRead = 0;
        while (totalRead < currentBlockSize) {
            ssize_t bytesRead = read(fd_in, buffer + totalRead, currentBlockSize - totalRead);
            if (bytesRead == -1) {
                fdWriteStr(2, "Error during read operation.\n");
                munmap(buffer, blockSize);
                close(fd_in);
                close(fd_out);
                _exit(1);
            }
            if (bytesRead == 0) {
                break; // EOF
            }
            totalRead += bytesRead;
        }

        if (totalRead == 0) {
            break;
        }

        // Reverse bytes in buffer
        for (ssize_t i = 0; i < totalRead / 2; i++) {
            char temp = buffer[i];
            buffer[i] = buffer[totalRead - i - 1];
            buffer[totalRead - i - 1] = temp;
        }

        ssize_t totalWritten = 0;
        while (totalWritten < totalRead) {
            ssize_t bytesWritten = write(fd_out, buffer + totalWritten, totalRead - totalWritten);
            if (bytesWritten == -1) {
                fdWriteStr(2, "Error during write operation.\n");
                munmap(buffer, blockSize);
                close(fd_in);
                close(fd_out);
                _exit(1);
            }
            totalWritten += bytesWritten;
        }

        blocksProcessed++;

        // Display progress on one line
        char progressMsg[32];
        int progressPercent = (int)((blocksProcessed * 100) / totalBlocks);
        int pos = 0;
        progressMsg[pos++] = '\r';
        const char* prefix = "Progress: ";
        for (int i = 0; prefix[i]; i++) {
            progressMsg[pos++] = prefix[i];
        }
        // Convert percentage to digits
        int p = progressPercent;
        char digits[4];
        int digitCount = 0;
        if (p == 0) {
            digits[digitCount++] = '0';
        } else {
            while (p > 0 && digitCount < 4) {
                digits[digitCount++] = (p % 10) + '0';
                p /= 10;
            }
        }
        // Reverse digits to get correct order
        for (int i = digitCount - 1; i >= 0; i--) {
            progressMsg[pos++] = digits[i];
        }
        progressMsg[pos++] = '%';
        progressMsg[pos] = '\0';

        write(1, progressMsg, pos);
    }

    write(1, "\n", 1);

    munmap(buffer, blockSize);
    close(fd_in);
    close(fd_out);
    return 0;
}
