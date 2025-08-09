#include <iostream>
#include <fcntl.h>      // open
#include <unistd.h>     // read, write, lseek, close
#include <sys/stat.h>   // stat, mkdir
#include <cstring>      // strerror
#include <cstdlib>      // exit, atoi

using namespace std;

void printUsage() {
    cerr << "Usage:\n";
    cerr << "./a.out <input_file> 0 <block_size>\n";
    cerr << "./a.out <input_file> 1\n";
    cerr << "./a.out <input_file> 2 <start_index> <end_index>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage();
        return 1;
    }

    char* inputFile = argv[1];
    int flag = atoi(argv[2]);

    if (flag == 0) {
        if (argc != 4) {
            cerr << "Error: Flag 0 requires <block_size>\n";
            printUsage();
            return 1;
        }
    } 
    else if (flag == 1) {
        if (argc != 3) {
            cerr << "Error: Flag 1 takes no extra args\n";
            printUsage();
            return 1;
        }
    } 
    else if (flag == 2) {
        if (argc != 5) {
            cerr << "Error: Flag 2 requires <start_index> <end_index>\n";
            printUsage();
            return 1;
        }
    } 
    else {
        cerr << "Error: Invalid flag (must be 0, 1, or 2)\n";
        printUsage();
        return 1;
    }

    cout << "Arguments parsed successfully.\n";
    return 0;
}