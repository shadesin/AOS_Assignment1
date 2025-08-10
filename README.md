# Assignment 1
### Souradeep Das - 2025201004


## Overview

This assignment has two main programs:

- **Q1**: Performs various types of file reversal using only Linux system calls.
- **Q2**: Validates the output files/directories created by Q1 - checks permissions and verifies content correctness for all reversal types.


***

## How to Compile

Open a terminal in the assignment folder and type:

`g++ 2025201004_A1_Q1.cpp -o q1`
`g++ 2025201004_A1_Q2.cpp -o q2`


***

## Q1 - File Reversal & Processing

### Usage

`./q1 <input_file> <flag> [extra arguments]`


- **flag 0 - Block-wise reversal:**
  - Reverses each block of the input file separately.
  - You must specify a positive integer block size.

`./q1 input.txt 0 1024`


- **flag 1 - Full file reversal:**
- Reverses the entire file (byte order).
- No extra arguments required.

`./q1 input.txt 1`


- **flag 2 - Partial range reversal:**
- Reverses the bytes before a start index and after an end index, keeping the region between [start, end] untouched.
- Requires two extra arguments: `start_index` and `end_index` (0-based indexing).

`./q1 input.txt 2 5 10`


### Output

- Output files are created in a sub-directory called **Assignment1**.
- Naming pattern: 
`Assignment1/<flag>_<inputfilename>`

For example, `Assignment1/0_file.txt`, `Assignment1/1_file.txt`, etc.

### Notes

- The program displays progress as a percentage.
- All errors (bad arguments, read/write failures, memory issues) are reported to the terminal.
- Only system calls are used. No commands like `ls`, `cp`, `mv`, or library I/O.

***

## Q2 - Validation & Permissions Checking

### Usage

For Flag 0:
`./q2 <new_file> <old_file> <dir_path> 0 <block_size>`

For Flag 1:
`./q2 <new_file> <old_file> <dir_path> 1`

For Flag 2:
`./q2 <new_file> <old_file> <dir_path> 2 <start_index> <end_index>`


#### Example (Flag 1):
`./q2 Assignment1/1_file.txt file.txt Assignment1 1`


### What Q2 Checks

- **Whether file sizes match** between new file and original.
- **Whether the reversal/copy transformation was performed correctly** (according to the flag).
- **Whether the output directory exists.**
- **Permissions checks:** User, group, and others - for `read`, `write`, and `execute` privileges on new file, old file, and directory.


***

## Code Structure & Comments

- **Manual helpers** are used for string/integer operations (`str_len`, `to_long`, `write_str`, etc.).
- **Memory allocation** is via `mmap` system call.
- **All output** (progress, errors, 30-line checker output) uses `write()` directly.
- **All permission checks** use only `stat()` and system macros.
- **Error handling** is explicit and robust for all system calls.

***

## Example Workflow

`./q1 file.txt 2 2 7`

Produces Assignment1/2_file.txt
`./q2 Assignment1/2_file.txt file.txt Assignment1 2 2 7`

Prints 30-line validation report

***

## Contact

For any clarifications, please contact - souradeep.das@students.iiit.ac.in.

***
