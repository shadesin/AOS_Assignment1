# Assignment 1 
**Author:** Souradeep Das – 2025201004  

This assigment contains two C++ programs written entirely using Linux system calls (`open`, `read`, `write`, `lseek`, `mmap`, `close`, etc.).  
No C++ standard library I/O functions (`cin`, `cout`, `fstream`, etc.) are used.

---

## Contents
- **Q1** – Performs various types of file reversal using only Linux system calls.
- **Q2** – Validates the output files/directories created by Q1 - checks permissions and verifies content correctness for all reversal types.

---

## Compilation

```bash
g++ 2025201004_A1_Q1.cpp -o q1
g++ 2025201004_A1_Q2.cpp -o q2
```

---

## Q1 – File Reversal & Processing  

### Usage
```bash
./q1 <input_file> <flag> [extra arguments]
```

### Flags
| Flag | Description | Extra Arguments |
|------|-------------|-----------------|
| `0`  | Block-wise reversal (each block reversed independently) | `<block_size>` |
| `1`  | Full file reversal | None |
| `2`  | Partial range reversal (before `start_index` and after `end_index` reversed, middle unchanged) | `<start_index> <end_index>` |

### Examples
```bash
# Block-wise reversal
./q1 input.txt 0 4

# Full reversal
./q1 input.txt 1

# Partial range reversal
./q1 input.txt 2 5 10
```

### Output
- All results are stored inside a directory named `Assignment1`.  
- File naming convention:  
  ```
  Assignment1/<flag>_<inputfilename>
  ```
- **Directory permissions:** `700` (owner: read, write, execute)  
- **File permissions:** `600` (owner: read, write)  

---

## Q2 – Output Validation & Permission Checks  

### Usage
```bash
# For flag 0
./q2 <new_file> <old_file> <dir_path> 0 <block_size>

# For flag 1
./q2 <new_file> <old_file> <dir_path> 1

# For flag 2
./q2 <new_file> <old_file> <dir_path> 2 <start_index> <end_index>
```

### What It Does
1. **Directory check** – Confirms if the given directory exists and is valid.  
2. **File size comparison** – Matches `new_file` and `old_file` sizes.  
3. **Content verification** –  
   - **Flag 0:** Each block in the new file is the reverse of the same block in the old file.  
   - **Flag 1:** Entire file content is reversed.  
   - **Flag 2:** Start and end segments reversed; middle section unchanged.  
4. **Permission checks** – Verifies expected permissions for:  
   - New file (`600`)  
   - Old file (default `644`)  
   - Directory (`700`)  

---

## Example Workflow
```bash
# Step 1 – Transform file
./q1 file.txt 2 5 10
# Output -> Assignment1/2_file.txt

# Step 2 – Validate transformation
./q2 Assignment1/2_file.txt file.txt Assignment1 2 5 10
```
Output will include:  
- Whether the directory exists
- File size match result  
- Content correctness check  
- 9 permission checks User, group, and others - for `read`, `write`, and `execute` privileges on new file, old file, and directory.

---

## Contact
For any clarifications, please contact:  
`souradeep.das@students.iiit.ac.in`