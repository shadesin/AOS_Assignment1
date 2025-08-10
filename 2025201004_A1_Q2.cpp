#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

// ---------- Simple utility functions ----------

// Get string length
int str_len(const char* s) { int i = 0; while (s[i]) i++; return i; }

// Write string
void write_str(int fd, const char* s) { write(fd, s, str_len(s)); }

// Write Yes/No
void write_yesno(int cond) { if (cond) write_str(1, "Yes\n"); else write_str(1, "No\n"); }

// Convert string to long
long to_long(const char* s) {
    long v = 0; int i = 0; if (!s[0]) return -1;
    while (s[i]) {
        if (s[i] < '0' || s[i] > '9') return -1;
        v = v * 10 + (s[i] - '0'); i++;
    }
    return v;
}

// Compare if buffer a is reverse of buffer b
int is_reverse(const char* a, const char* b, long n) {
    for (long i = 0; i < n; i++) if (a[i] != b[n - 1 - i]) return 0;
    return 1;
}

// Print permissions for an entity (new file, old file, directory)
void print_perms_for(const char* name, struct stat *st) {
    write_str(1, "User has read permissions on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IRUSR);
    write_str(1, "User has write permission on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IWUSR);
    write_str(1, "User has execute permission on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IXUSR);

    write_str(1, "Group has read permissions on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IRGRP);
    write_str(1, "Group has write permission on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IWGRP);
    write_str(1, "Group has execute permission on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IXGRP);

    write_str(1, "Others have read permissions on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IROTH);
    write_str(1, "Others have write permission on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IWOTH);
    write_str(1, "Others have execute permission on "); write_str(1, name); write_str(1, ": "); write_yesno(st->st_mode & S_IXOTH);
}

// ---------- Content checkers for each flag ----------

int check_flag0(const char* nf, const char* of, long block) {
    int fn = open(nf, O_RDONLY), fo = open(of, O_RDONLY);
    if (fn < 0 || fo < 0) return 0;
    char* b1 = (char*)mmap(NULL, block, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char* b2 = (char*)mmap(NULL, block, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (b1 == MAP_FAILED || b2 == MAP_FAILED) return 0;
    int ok = 1; ssize_t r1, r2;
    while (1) {
        r1 = read(fn, b1, block);
        r2 = read(fo, b2, block);
        if (r1 != r2) { ok = 0; break; }
        if (r1 <= 0) break;
        if (!is_reverse(b1, b2, r1)) { ok = 0; break; }
    }
    munmap(b1, block); munmap(b2, block);
    close(fn); close(fo);
    return ok;
}

int check_flag1(const char* nf, const char* of, long chunk) {
    int fn = open(nf, O_RDONLY), fo = open(of, O_RDONLY);
    if (fn < 0 || fo < 0) return 0;
    struct stat st; if (stat(nf, &st) < 0) return 0;
    long size = st.st_size, offset = 0;
    char* b1 = (char*)mmap(NULL, chunk, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char* b2 = (char*)mmap(NULL, chunk, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    int ok = 1;
    while (offset < size) {
        long sz = (chunk < (size - offset)) ? chunk : (size - offset);
        lseek(fo, offset, SEEK_SET); read(fo, b2, sz);
        lseek(fn, size - offset - sz, SEEK_SET); read(fn, b1, sz);
        if (!is_reverse(b1, b2, sz)) { ok = 0; break; }
        offset += sz;
    }
    munmap(b1, chunk); munmap(b2, chunk);
    close(fn); close(fo);
    return ok;
}

int check_flag2(const char* nf, const char* of, long start, long end, long chunk) {
    struct stat st; if (stat(of, &st) < 0) return 0;
    long size = st.st_size;
    if (start >= size || end >= size || start >= end) return 0;
    int fn = open(nf, O_RDONLY), fo = open(of, O_RDONLY);
    if (fn < 0 || fo < 0) return 0;
    char* b1 = (char*)mmap(NULL, chunk, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char* b2 = (char*)mmap(NULL, chunk, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    int ok = 1;

    // Before start
    long len1 = start, off = 0;
    while (off < len1 && ok) {
        long sz = (chunk < (len1 - off)) ? chunk : (len1 - off);
        lseek(fn, off, SEEK_SET); read(fn, b1, sz);
        lseek(fo, len1 - off - sz, SEEK_SET); read(fo, b2, sz);
        if (!is_reverse(b1, b2, sz)) ok = 0;
        off += sz;
    }

    // Middle unchanged
    long len2 = end - start + 1; off = 0;
    while (ok && off < len2) {
        long sz = (chunk < (len2 - off)) ? chunk : (len2 - off);
        lseek(fn, start + off, SEEK_SET); read(fn, b1, sz);
        lseek(fo, start + off, SEEK_SET); read(fo, b2, sz);
        for (long i = 0; i < sz; i++) if (b1[i] != b2[i]) { ok = 0; break; }
        off += sz;
    }

    // After end
    long len3 = size - end - 1; off = 0;
    while (ok && off < len3) {
        long sz = (chunk < (len3 - off)) ? chunk : (len3 - off);
        lseek(fn, end + 1 + off, SEEK_SET); read(fn, b1, sz);
        lseek(fo, size - (off + sz), SEEK_SET); read(fo, b2, sz);
        if (!is_reverse(b1, b2, sz)) ok = 0;
        off += sz;
    }

    munmap(b1, chunk); munmap(b2, chunk);
    close(fn); close(fo);
    return ok;
}

// ---------- Main ----------
int main(int argc, char* argv[]) {
    if (argc < 5) { write_str(2, "Invalid args\n"); _exit(1); }

    const char* newfile = argv[1];
    const char* oldfile = argv[2];
    const char* dirpath  = argv[3];
    int flag = (int)to_long(argv[4]);

    long block = 0, start = 0, end = 0;
    if (flag == 0) { if (argc != 6) _exit(1); block = to_long(argv[5]); }
    else if (flag == 1) { if (argc != 5) _exit(1); block = 1024*1024; }
    else if (flag == 2) { if (argc != 7) _exit(1); start = to_long(argv[5]); end = to_long(argv[6]); block = 1024*1024; }
    else _exit(1);

    struct stat st_new, st_old, st_dir;
    int new_ok = (stat(newfile, &st_new) == 0);
    int old_ok = (stat(oldfile, &st_old) == 0);
    int dir_ok = (stat(dirpath, &st_dir) == 0);

    // 1. Directory created
    write_str(1, "Directory is created: "); write_yesno(dir_ok);

    // 2. Content correct
    int content_ok = 0;
    if (flag == 0) content_ok = (new_ok && old_ok) ? check_flag0(newfile, oldfile, block) : 0;
    else if (flag == 1) content_ok = (new_ok && old_ok) ? check_flag1(newfile, oldfile, block) : 0;
    else if (flag == 2) content_ok = (new_ok && old_ok) ? check_flag2(newfile, oldfile, start, end, block) : 0;
    write_str(1, "Whether file contents are correctly processed: "); write_yesno(content_ok);

    // 3. Both files sizes same
    int size_same = (new_ok && old_ok && st_new.st_size == st_old.st_size);
    write_str(1, "Both Files Sizes are Same: "); write_yesno(size_same);

    // Permissions - new file
    if (new_ok) print_perms_for("new file", &st_new);
    else for (int i = 0; i < 9; i++) write_yesno(0);

    // Permissions - old file
    if (old_ok) print_perms_for("old file", &st_old);
    else for (int i = 0; i < 9; i++) write_yesno(0);

    // Permissions - directory
    if (dir_ok) print_perms_for("directory", &st_dir);
    else for (int i = 0; i < 9; i++) write_yesno(0);

    return 0;
}
