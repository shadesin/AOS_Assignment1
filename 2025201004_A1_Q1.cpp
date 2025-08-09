#include <fcntl.h>      // open
#include <unistd.h>     // read, write, lseek, close
#include <sys/stat.h>   // stat, mkdir
#include <cstring>      // strlen, strerror
#include <cstdlib>      // exit, atoi
#include <errno.h>      // errno
#include <new>          // nothrow

// Helper: write string to fd
void writeStr(int fd, const char* s) {
    write(fd, s, strlen(s));
}

// Helper: print usage (to stderr)
void printUsage() {
    writeStr(2, "Usage:\n");
    writeStr(2, "./q1 <input_file> 0 <block_size>\n");
    writeStr(2, "./q1 <input_file> 1\n");
    writeStr(2, "./q1 <input_file> 2 <start_index> <end_index>\n");
}

// Helper: write an integer to fd (for progress)
void writeInt(int fd, int num) {
    char buf[12];
    int i = 10; buf[11] = '\0';
    if (num == 0) { buf[10] = '0'; write(fd, buf+10, 1); return; }
    bool neg = false;
    if (num < 0) { neg = true; num = -num; }
    while(num > 0 && i>=0) { buf[i--] = '0'+(num%10); num/=10; }
    if (neg && i>=0) buf[i--] = '-';
    write(fd, buf+i+1, 11-i-1);
}

int main(int argc, char* argv[]) {
    // Argument parsing using only system calls
    if (argc < 3) {
        printUsage();
        _exit(1);
    }

    char* inputFile = argv[1];
    int flag = atoi(argv[2]);

    if (flag == 0) {
        if (argc != 4) {
            writeStr(2, "Error: Flag 0 requires <block_size>\n");
            printUsage();
            _exit(1);
        }
    } else {
        writeStr(2, "Only flag 0 (block-wise reversal) is implemented in this version!\n");
        printUsage();
        _exit(1);
    }

    // Create Assignment1 directory
    if (mkdir("Assignment1", 0700) == -1) {
        if (errno != EEXIST) {
            writeStr(2, "mkdir failed: ");
            writeStr(2, strerror(errno));
            writeStr(2, "\n");
            _exit(1);
        }
    }

    // Open input file first
    int in_fd = open(inputFile, O_RDONLY);
    if (in_fd == -1) {
        writeStr(2, "open input file failed: ");
        writeStr(2, strerror(errno));
        writeStr(2, "\n");
        _exit(1);
    }

    // Create output file
    int out_fd = open("Assignment1/output_file", O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (out_fd == -1) {
        writeStr(2, "open output file failed: ");
        writeStr(2, strerror(errno));
        writeStr(2, "\n");
        close(in_fd);
        _exit(1);
    }

    // Get input file size
    off_t fileSize = lseek(in_fd, 0, SEEK_END);
    if (fileSize == (off_t)-1) {
        writeStr(2, "lseek failed\n");
        close(in_fd); close(out_fd);
        _exit(1);
    }
    if (lseek(in_fd, 0, SEEK_SET) == (off_t)-1) {
        writeStr(2, "lseek reset failed\n");
        close(in_fd); close(out_fd);
        _exit(1);
    }

    int blockSize = atoi(argv[3]);
    if (blockSize <= 0) {
        writeStr(2, "Error: Block size must be positive\n");
        close(in_fd); close(out_fd);
        _exit(1);
    }
    if (blockSize > 8 * 1024 * 1024) {
        writeStr(2, "Warning: Block size > 8MB. Consider smaller size.\n");
    }

    char* buffer = new(std::nothrow) char[blockSize];
    if (buffer == nullptr) {
        writeStr(2, "Error: Failed to allocate buffer memory\n");
        close(in_fd); close(out_fd);
        _exit(1);
    }

    off_t totalBlocks = (fileSize + blockSize - 1) / blockSize;
    off_t processedBlocks = 0;
    while (processedBlocks < totalBlocks) {
        ssize_t currentBlockSize = blockSize;
        off_t bytesLeft = fileSize - processedBlocks * blockSize;
        if (bytesLeft < blockSize) currentBlockSize = bytesLeft;

        // Read block, in a loop for robustness
        ssize_t bytesRead = 0;
        while (bytesRead < currentBlockSize) {
            ssize_t r = read(in_fd, buffer + bytesRead, currentBlockSize - bytesRead);
            if (r == -1) {
                writeStr(2, "read failed: ");
                writeStr(2, strerror(errno));
                writeStr(2, "\n");
                delete[] buffer;
                close(in_fd); close(out_fd);
                _exit(1);
            }
            if (r == 0) break; // EOF
            bytesRead += r;
        }
        if (bytesRead == 0) break; // End of file

        // Reverse block in place
        for (ssize_t i = 0; i < bytesRead / 2; ++i) {
            char t = buffer[i];
            buffer[i] = buffer[bytesRead - i - 1];
            buffer[bytesRead - i - 1] = t;
        }

        // Write block, loop for robustness
        ssize_t bytesWritten = 0;
        while (bytesWritten < bytesRead) {
            ssize_t w = write(out_fd, buffer + bytesWritten, bytesRead - bytesWritten);
            if (w == -1) {
                writeStr(2, "write failed: ");
                writeStr(2, strerror(errno));
                writeStr(2, "\n");
                delete[] buffer;
                close(in_fd); close(out_fd);
                _exit(1);
            }
            bytesWritten += w;
        }

        processedBlocks++;

        // Print progress with carriage return using only write()
        char progressStr[30];
        int percent = (int)((processedBlocks * 100) / totalBlocks);
        // Manually build progress string: "\rProgress: " + int(percent) + "%"
        int idx = 0;
        progressStr[idx++] = '\r';
        const char* pre = "Progress: ";
        for (int j=0; pre[j]; ++j) progressStr[idx++] = pre[j];
        // Write percent
        int p = percent, digits[4], dcount=0;
        if (p==0) digits[dcount++]=0;
        while(p>0 && dcount<4) { digits[dcount++] = p%10; p/=10;}
        for(int j=dcount-1;j>=0;j--) progressStr[idx++] = '0'+digits[j];
        progressStr[idx++] = '%';
        progressStr[idx] = '\0';
        write(1, progressStr, idx);
    }
    writeStr(1, "\n");

    delete[] buffer;
    close(in_fd);
    close(out_fd);
    return 0;
}
