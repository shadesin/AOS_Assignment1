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
    // Argument parsing
    if (argc < 3) { printUsage(); _exit(1); }

    const char* inputFile = argv[1];
    int mode = convertToInt(argv[2]);

    if (mode == 0) {
        if (argc != 4) {
            fdWriteStr(2, "Error: Flag 0 requires block size.\n");
            printUsage();
            _exit(1);
        }
    } else {
        fdWriteStr(2, "Only flag 0 implemented in this version.\n");
        _exit(1);
    }

    int blockSize = convertToInt(argv[3]);
    if (blockSize <= 0) {
        fdWriteStr(2, "Invalid block size.\n");
        _exit(1);
    }
    if (blockSize > 8 * 1024 * 1024) {
        fdWriteStr(2, "Warning: block size > 8MB.\n");
    }

    // Create Assignment1 directory
    if (mkdir("Assignment1", 0700) == -1 && errno != EEXIST) {
        fdWriteStr(2, "mkdir Assignment1 failed\n");
        _exit(1);
    }

    // Open input file
    int fd_in = open(inputFile, O_RDONLY);
    if (fd_in == -1) { fdWriteStr(2, "Open input failed\n"); _exit(1); }

    // Build dynamic output path: Assignment1/<flag>_<basename>
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

    // Create output file
    int fd_out = open(outputPath, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd_out == -1) { fdWriteStr(2, "Open output failed\n"); close(fd_in); _exit(1); }

    // File size
    off_t fileSize = lseek(fd_in, 0, SEEK_END);
    if (fileSize < 0) { fdWriteStr(2, "lseek fail\n"); close(fd_in); close(fd_out); _exit(1); }
    lseek(fd_in, 0, SEEK_SET);

    // Allocate buffer with mmap
    char* buffer = (char*) mmap(NULL, blockSize, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
        fdWriteStr(2, "Buffer alloc fail\n");
        close(fd_in); close(fd_out);
        _exit(1);
    }

    off_t totalBlocks = (fileSize + blockSize - 1) / blockSize;
    off_t doneBlocks = 0;

    while (doneBlocks < totalBlocks) {
        ssize_t chunk = blockSize;
        off_t left = fileSize - doneBlocks * blockSize;
        if (left < blockSize) chunk = left;

        ssize_t rbytes = 0;
        while (rbytes < chunk) {
            ssize_t r = read(fd_in, buffer + rbytes, chunk - rbytes);
            if (r <= 0) break;
            rbytes += r;
        }
        if (rbytes == 0) break;

        // Reverse in place
        for (ssize_t i = 0; i < rbytes / 2; i++) {
            char t = buffer[i];
            buffer[i] = buffer[rbytes - i - 1];
            buffer[rbytes - i - 1] = t;
        }

        ssize_t wbytes = 0;
        while (wbytes < rbytes) {
            ssize_t w = write(fd_out, buffer + wbytes, rbytes - wbytes);
            if (w <= 0) break;
            wbytes += w;
        }

        doneBlocks++;

        // Progress output
        write(1, "\rProgress: ", 11);
        fdWriteInt(1, (int)((doneBlocks * 100) / totalBlocks));
        write(1, "%", 1);
    }

    write(1, "\n", 1);

    munmap(buffer, blockSize);
    close(fd_in);
    close(fd_out);
    return 0;
}
