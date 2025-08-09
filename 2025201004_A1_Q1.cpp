#include <fcntl.h>      // open
#include <unistd.h>     // read, write, lseek, close, _exit
#include <sys/stat.h>   // mkdir
#include <errno.h>      // errno
#include <sys/mman.h>   // mmap, munmap
#include <sys/types.h>  // size_t, off_t

// Manual strlen using only pointer math
size_t my_strlen(const char* s) {
    size_t len = 0;
    while (s[len] != '\0') len++;
    return len;
}

// Manual string to integer (positive only for block size & flag)
int strToInt(const char* s) {
    int val = 0;
    int i = 0;
    if(s[0] == '-') return -1; // simple negative detect
    while (s[i] >= '0' && s[i] <= '9') {
        val = val * 10 + (s[i] - '0');
        i++;
    }
    if (s[i] != '\0') return -1; // invalid char
    return val;
}

void writeStr(int fd, const char* s) {
    write(fd, s, my_strlen(s));
}

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

void usage() {
    writeStr(2,"Usage:\n");
    writeStr(2,"./q1 <input> 0 <block>\n");
}

int main(int argc, char* argv[]) {
    if (argc != 4) { usage(); _exit(1); }

    int flag = strToInt(argv[2]);
    if (flag != 0) {
        writeStr(2,"Only flag 0 implemented\n");
        _exit(1);
    }

    int blockSize = strToInt(argv[3]);
    if (blockSize <= 0) {
        writeStr(2,"Invalid block size\n");
        _exit(1);
    }

    // mkdir
    if (mkdir("Assignment1",0700) == -1 && errno != EEXIST) {
        writeStr(2,"mkdir failed\n");
        _exit(1);
    }

    // open input
    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd == -1) { writeStr(2,"input open fail\n"); _exit(1); }

    // open output
    int out_fd = open("Assignment1/output_file", O_CREAT|O_WRONLY|O_TRUNC, 0600);
    if (out_fd == -1) { writeStr(2,"output open fail\n"); close(in_fd); _exit(1); }

    off_t fileSize = lseek(in_fd, 0, SEEK_END);
    if (fileSize < 0) { writeStr(2,"lseek fail\n"); _exit(1); }
    lseek(in_fd,0,SEEK_SET);

    // allocate buffer with mmap (pure syscall)
    char* buffer = (char*) mmap(NULL, blockSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) { writeStr(2,"alloc fail\n"); _exit(1); }

    off_t totalBlocks = (fileSize + blockSize - 1) / blockSize;
    off_t processed = 0;

    while (processed < totalBlocks) {
        ssize_t chunk = blockSize;
        off_t left = fileSize - processed * blockSize;
        if (left < blockSize) chunk = left;

        ssize_t rbytes = 0;
        while (rbytes < chunk) {
            ssize_t r = read(in_fd, buffer + rbytes, chunk - rbytes);
            if (r <= 0) break;
            rbytes += r;
        }
        if (rbytes == 0) break;

        for (ssize_t i=0;i<rbytes/2;i++) {
            char t = buffer[i];
            buffer[i] = buffer[rbytes-i-1];
            buffer[rbytes-i-1] = t;
        }

        ssize_t wbytes = 0;
        while (wbytes < rbytes) {
            ssize_t w = write(out_fd, buffer + wbytes, rbytes - wbytes);
            if (w <= 0) break;
            wbytes += w;
        }

        processed++;

        // progress: \rProgress: XX%
        write(1,"\rProgress: ",11);
        writeInt(1,(int)((processed*100)/totalBlocks));
        write(1,"%",1);
    }

    write(1,"\n",1);

    munmap(buffer, blockSize);
    close(in_fd);
    close(out_fd);
    return 0;
}
