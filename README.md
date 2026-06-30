# Dynamic Memory Analysis Tool using Intel PIN

## Overview

This project implements a dynamic memory analysis tool using **Intel PIN**, a dynamic binary instrumentation framework.

The tool instruments a target program at runtime and monitors heap memory operations such as:

* `malloc`
* `calloc`
* `realloc`
* `free`

It generates a detailed memory trace and automatically detects common memory management issues including:

* Memory Leaks
* Double Free Errors
* Invalid Free Operations

The project demonstrates how binary instrumentation can be used to build lightweight runtime debugging and memory analysis tools without modifying the source code of the target application.

---

## Features

### Allocation Tracking

Tracks all dynamic memory allocations performed using:

* `malloc()`
* `calloc()`
* `realloc()`

For every allocation, the tool records:

* Memory address
* Allocation size
* Allocation type

---

### Deallocation Tracking

Tracks all calls to:

* `free()`

Valid frees are recorded and matched against previously allocated memory blocks.

---

### Memory Leak Detection

At program termination, the tool reports memory blocks that were allocated but never released.

Example:

```text
Memory Leaks Detected:
Leak at 0x7f92a0012340 Size: 64 bytes
Leak at 0x7f92a0015670 Size: 80 bytes
```

---

### Double Free Detection

Detects attempts to free the same memory block multiple times.

Example:

```text
[DOUBLE FREE DETECTED] Address: 0x55ab34c1d2a0
```

---

### Invalid Free Detection

Detects attempts to free memory that was never allocated dynamically.

Example:

```text
[INVALID FREE DETECTED] Address: 0x7ffd5a12c4f0
```

---

### Detailed Statistics

The tool generates a summary containing:

* Number of malloc calls
* Number of calloc calls
* Number of realloc calls
* Number of valid frees
* Number of invalid frees
* Number of double frees
* Number of memory leaks

---

## Project Structure

```text
.
├── MyPinTool.cpp      # Intel PIN memory analysis tool
├── testcase.c         # Test application
├── memtrace.out       # Generated memory trace output
└── README.md
```

---

## How It Works

### Step 1: Instrumentation

The tool intercepts runtime calls to:

```c
malloc()
calloc()
realloc()
free()
```

using Intel PIN routine instrumentation.

---

### Step 2: Allocation Database

Every allocation is stored in an internal map:

```cpp
active_allocs[address] = size;
```

This allows the tool to maintain a live view of all active heap allocations.

---

### Step 3: Error Detection

When `free()` is invoked:

* If the pointer exists in active allocations → Valid Free
* If the pointer was already freed → Double Free
* If the pointer was never allocated → Invalid Free

---

### Step 4: Leak Analysis

At program termination, all remaining active allocations are reported as memory leaks.

---

## Building the Tool

Compile the PIN tool inside the Intel PIN source tree:

```bash
make obj-intel64/MyPinTool.so TARGET=intel64
```

---

## Running the Tool

Compile the test program:

```bash
gcc testcase.c -o testcase
```

Run with Intel PIN:

```bash
pin -t obj-intel64/MyPinTool.so -- ./testcase
```

Output is generated in:

```text
memtrace.out
```

---

## Sample Output

```text
[MALLOC] Address: 0x55a4c2d6d2a0 | Size: 28 bytes

[FREE] Address: 0x55a4c2d6d2a0

[DOUBLE FREE DETECTED] Address: 0x55a4c2d6d2a0

[INVALID FREE DETECTED] Address: 0x7ffd12345678

===== FINAL REPORT =====

Memory Leaks Detected:
Leak at 0x55a4c2d6d500 Size: 64 bytes

===== STATISTICS =====

Malloc Calls       : 6
Calloc Calls       : 2
Realloc Calls      : 2
Valid Frees        : 4
Invalid Frees      : 1
Double Frees       : 1
Memory Leaks       : 3
```

---

## Test Cases Included

The provided test application covers:

* Valid malloc/free
* Memory leak scenarios
* Partial memory leaks
* Calloc allocation tracking
* Calloc leaks
* Realloc tracking
* Realloc leaks
* Double free detection
* Invalid free detection

---

## Technologies Used

* C++
* Intel PIN
* Dynamic Binary Instrumentation (DBI)
* GCC
* Linux

---

## Future Improvements

* Use-after-free detection
* Heap corruption detection
* Memory access tracing
* Per-thread allocation statistics
* Visualization dashboard
* Call stack collection for leak origins

---

## Author

Malothu Jeswanth
B.Tech CSE, IIT Ropar

Project developed as part of learning and experimentation with Intel PIN and dynamic binary instrumentation.
