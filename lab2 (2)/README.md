# Lab 2: Kernel Data Structure

## Assignment Information
- **Handed out:** Tuesday, Sep. 30, 2025
- **Due:** Friday, Oct. 17, 2025 (23:59 Europe/Zurich)
- **Submission Requirements:** 
  - Submit a single ZIP file containing:
    - Your kernel module
    - A markdown report (for Part 3)
    - **Do NOT include** the Linux kernel source code or the user program we provide in Part 3.

---

## Overview:
In this lab, you will gain hands-on experience with essential
Linux kernel data structures. You will create a kernel module
that implements a `/proc` filesystem interface, allowing userspace
programs to interact with various kernel data structures including:

- Linked lists
- Hash tables  
- Red-black trees
- XArrays

Through this interface, users will be able to:
- Add data to these structures
- Delete data from these structures
- View the current contents of these structures

---

## Recommended Background Reading
You may find the following resources useful for your lab. This lab
covers the contents from both lectures and tutorials presented in 
the third and fourth week.

### `procfs`
- [Linux /proc file system](http://pointer-overloading.blogspot.com/2013/09/linux-creating-entry-in-proc-file.html)
- [Sample /proc implementation in tutorial 03](https://github.com/PanJason/module_and_ebpf/blob/521ecd22223105f591924c70509ebfbc417f6dc5/kernel_module_examples/wake_up/kernel/sleep_fs.c#L189)

### Data Structures
- [Linked list](https://docs.kernel.org/core-api/list.html)
- [Hash table](https://kernelnewbies.org/FAQ/Hashtables)
- [Xarray](https://docs.kernel.org/core-api/xarray.html)
- [RB tree](https://docs.kernel.org/core-api/rbtree.html)

### Synchronization Primitives
- [Spinlock](https://www.kernel.org/doc/Documentation/locking/spinlocks.txt)
- [RCU](https://docs.kernel.org/6.4/RCU/whatisRCU.html)

### Shell
- [Bash Scripting Tutorial for Beginners](https://linuxconfig.org/bash-scripting-tutorial-for-beginners)
- [Bash scripting Tutorial](https://linuxconfig.org/bash-scripting-tutorial)
- [Bash Guide for Beginners](http://tldp.org/LDP/Bash-Beginners-Guide/html/)
- [Explain Shell](https://explainshell.com/)

---

# Part 1. Creating a `procfs` interface [30 pts]
## Objective
Create a kernel module called `lab2.ko` which adds a `procfs` file called
`/proc/lab2` with read/write capabilities.

## Module Initialization
### Module Parameter
You module has to accept a parameter called `int_str` which is a string of 
integers.
- **Format:** A comma-separated string of integers
- **Maximum:** 256 integers
- **Example:** `sudo insmod lab2.ko int_str=1,2,3,4,5`

### Error Handling for Initialization
- **Valid inputs:** 
  - Comma-separated integers (e.g., `int_str=1,2,3`)
  - Empty string (e.g., `int_str=`)
  - Not specified (e.g., `sudo insmod lab2.ko`)
- **Invalid inputs:** 
  - Any other format (e.g., `int_str=1.2.3` or `int_str=abc`)
  - If invalid input is provided:
    - Module should still load successfully
    - Print `INVALID_INIT_ARGUMENT` to dmesg

### Requirement
In this lab, you are **not allowed** to use [seq file] in the implementation.

## Write Operation
The `/proc/lab2` file accepts write commands in this format:
```
<OPERATION> <COMMA_SEPARATED_INTEGERS>
```
where `<COMMA_SEPARATED_INTEGERS>` is a comma-separated string of integers,
with a maximum of 256 integers, which has the same format as `int_str`.

### Valid Operations
- `a` - Add operation
- `d` - Delete operation

### Valid Examples
```bash
$ echo "d 1,2,3" | sudo tee /proc/lab2    # Delete integers 1, 2, and 3
$ echo "d 2" | sudo tee /proc/lab2        # Delete integer 2
$ echo "a 1,2" | sudo tee /proc/lab2      # Add integers 1 and 2
```

### Special Case
```bash
$ echo "a 1 2 3" | sudo tee /proc/lab2    
# Treated as "a 1" (only first integer after operation is considered)
```

### Invalid Examples
```bash
$ echo "d" | sudo tee /proc/lab2          # Missing integers
$ echo "x 232" | sudo tee /proc/lab2      # Invalid operation 'x'
$ echo "" | sudo tee /proc/lab2           # Empty string
```

### Error Handling for Write
- **Valid write:** Save the command for later retrieval (no output to dmesg)
- **Invalid write:** Print `INVALID ARGUMENT` to dmesg

## Read Operation
When reading `/proc/lab2`, the module should output to **dmesg** (not to 
the userland buffer). Note that you can use `printk` safely in any context.

### Output Rules
1. Print the last **valid** write command that your kernel module received.
2. If no valid write has occurred:
   - Print `int_str` value if it was valid during initialization.
   - Print nothing if `int_str` was invalid

### Example Sequence
```bash
$ sudo insmod lab2.ko int_str=1,2,3       # Load module
$ echo "d 1,2,3" | sudo tee /proc/lab2    # Valid write
$ echo "xx" | sudo tee /proc/lab2         # Invalid write (ignored)
$ cat /proc/lab2                          # Read operation
# Check dmesg - should show: "d 1,2,3"
```

## Grading (30 points)
- Module loading/unloading: 10 points
- Write operation correctness: 10 points
- Read operation correctness: 10 points

---

# Part 2. Adding data structures to the kernel module [25 pts]
## Objective
In this part you will extend your kernel module to support the following 
data structures.
- Linked list
- Hash table
- Red-black tree
- XArray

The data structures are covered in [Lecture 03](https://moodle.epfl.ch/pluginfile.php/3481626/mod_resource/content/5/Kernel%20data%20structures%20and%20synchronization.pdf).

## Module Initialization
### New Module Parameter
Add a `ds` parameter to specify the data structure type:

| Parameter Value | Data Structure        |
|-----------------|-----------------------|
| `linked_list`   | Linked List (default) |
| `hash_table`    | Hash Table            |
| `rb_tree`       | Red-Black Tree        |
| `xarray`        | XArray                |

**Note:** If `ds` is not specified or is `NULL`, default to `linked_list`. If
an invalid `ds` value is provided, print `INVALID_INIT_ARGUMENT` to dmesg.
We will not test any reads/writes to the procfs file with an invalid `ds` input.

### Initialization Behavior
When the module loads:
1. Initialize the specified data structure
2. Add all integers from `int_str` to the data structure

**Example:**
```bash
$ sudo insmod lab2.ko int_str=1,3 ds=linked_list
# Creates a linked list containing integers 1 and 3
```

## Write Operation
Your kernel module now needs to parse the string written by user and performs
the actions accordingly. It should perform the following two kind of operations.
### Add Operation (`a`)
- Add specified integers to the data structure
- **Only add if the integer does NOT already exist**
- Example: `echo "a 1,2,3" | sudo tee /proc/lab2`
- This will add number 1,2 and 3 to the initialized data structure if they 
don't exist already.

### Delete Operation (`d`)
- Remove specified integers from the data structure
- **Only delete if the integer EXISTS**
- Example: `echo "d 1,2,3" | sudo tee /proc/lab2`
- This will delete number 1,2 and 3 from the initialized data structure if they exist.

**Note:** No output to dmesg for valid operations.

## Read Operation
You kernel module needs to print the list of numbers in the 
data structure to `dmesg` after the last valid argument by write operation (part 1). 
### Output Format
```
<last_valid_command>
<data_structure_prefix>
<comma_separated_integers>
```
The first line is the same as Part 1.

### Data Structure Prefix
- Linked list: `Linked list:`
- Hash table: `Hash table:`
- Red-black tree: `Red-black tree:`
- XArray: `Xarray:`

### Formatting Rules for Comma Separated Integers
- Maximum 256 numbers per line
- No comma after the last number on each line
- Split to multiple lines if the data structure contains more than 256 numbers.

### Example
```bash
$ sudo insmod lab2.ko int_str=1,2,3,4,5 ds=linked_list
$ echo "d 1,2,3,6" | sudo tee /proc/lab2
$ cat /proc/lab2
```

Expected dmesg output:
```
d 1,2,3,6
Linked list:
4,5
```

## Memory safety
You kernel module must safely free the memory taken by data structures 
when:
- an element is deleted
- the kernel module exits

## Grading (25 points)
- Memory freed safely in exit and error handling: 5 points
- Linked list implementation: 5 points
- Hash table implementation: 5 points
- Red-black tree implementation: 5 points
- XArray implementation: 5 points

---

# Part 3. Make data structure concurrency safe [30 pts]

## Objective
Previously we have implemented our data structure in a single-threaded 
fashion but multiple users can still corrupt the data. So in 
this part, you will make some of the data structures corrently safe 
with different synchronization primitives. One of them, `xarray`, 
is already concurrency safe. You need to make two of the remaining three,
linked list and hash table, also concurrency safe.

## Synchronization Requirement
You kernel module will protect the following data structures
- `Linked list`
- `Hash table`

by following two synchronization mechanisms:

1. `spinlock`
2. `RCU`

The synchronization primitives are covered in [Lecture 04](https://moodle.epfl.ch/pluginfile.php/3484632/mod_resource/content/7/04-sync2.pdf),
the [Week 4 tutorial](https://docs.google.com/presentation/d/1KG_7w08Rb9IzXvfzxGtLN2OrEdKyqvH7/), and the [Week 4 Tutorial Source Code](https://github.com/sidharth-sundar/advos-25-demo/tree/main/Week-4-Demo).

## Module Initialization
### New Module Parameter
Add a `sync` parameter to specify the synchronization mechanism as follows:
```bash
# Using spinlock
$ sudo insmod lab2.ko int_str=1,2,3,4,5 sync=spinlock ds=linked_list

# Using RCU
$ sudo insmod lab2.ko int_str=1,2,3,4,5 sync=rcu ds=linked_list
```

If an invalid `sync` value is provided, print `INVALID_INIT_ARGUMENT` to dmesg.
We will not test any reads/writes to the procfs file with an invalid `sync` input.

## Test Your Implementation with Userland Program
After changing the data structure to be concurrency safe, you need to 
use the userspace program called `lab2_test.c`
### Test Program Behavior
`lab2_test` that we provide in `userland` executes the following steps:
1. Load the kernel module with the synchronization mechanism specified by the
argument `-s` and the data structure specified by the argument `-d`.
1. Start 4 threads concurrently
2. Each thread performs 10000 read or write operations to `/proc/lab2` with
   read/write ratios specified by the argument `-r`
4. For all the write operation, keep add/delete ratio to 50:50
5. For each write operation, either add or delete just one random int, for 
    example `echo "a 1" | sudo tee /proc/lab2` or `echo "d 2" | sudo tee /proc/lab2`
6. Unload the kernel module

### Test Program Compilation
To compile the userland program,
```bash
cd userland
make
```

### Test Program Usage
The usage of the program is as follows:
```bash
$ ./lab2_test -h
Usage: ./lab2_test -r <read_ratio> -s <sync_mechanism> -k <kernel_module_path> -d <data_structure>

Options:
    -r <read_ratio>       Ratio of read operations (0–100).
                          Example: -r 70 means 70% reads, 30% writes.
    -s <sync_mechanism>   Synchronization mechanism to use.
                          Available choices:
                            spinlock  Use spinlocks
                            rcu       Use RCU
    
    -k <kernel_module_path> Path to the lab2 kernel module.
    -d <data_structure>   Data structure to use.
                          Available choices:
                            linked_list
                            hash_table
                            rb_tree
                            xarray


Example:
    ./lab2_test -r 90 -s rcu -k /lib/modules/...lab2.ko -d linked_list
```

### Hardware Requirements for Test Program
Since the our `lab2_test` by default launches 4 different threads, 
you should start your vm with 4 vCPUs to each thread to be 
scheduled on one vCPU.

## Report Performance
For this part, you need to write a report to describe
-  The speed of your concurrent data structure in ops/s, which means the
   speed to finish all 40000 (4x10000) operations, with three different r/w
   ratios (10:90, 50:50, 90:10, 99:1) in a table as follows:

`Linked list:`
| R/W ratio | 10:90   | 50:50   | 90:10   | 99:1    |
| --------  | -----   | -----   | -----   | ----    |
| Spinlock  | a ops/s | b ops/s | c ops/s | d ops/s |
| RCU       | e ops/s | f ops/s | g ops/s | h ops/s |

`Hash table:`
| R/W ratio | 10:90   | 50:50   | 90:10   | 99:1    |
| --------  | -----   | -----   | -----   | ----    |
| Spinlock  | a ops/s | b ops/s | c ops/s | d ops/s |
| RCU       | e ops/s | f ops/s | g ops/s | h ops/s |

-  Explain the reason why the results are as above.
-  The tradeoff between spinlock and RCU. What synchronization 
primitive one should use under different scenarios?

## Grading (30 points)
- Spinlock implementation (both data structures): 10 points
- RCU implementation (both data structures): 10 points
- Performance report and analysis: 10 points

---

