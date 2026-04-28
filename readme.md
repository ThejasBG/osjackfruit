# Create README.md file with the provided content

content = """# Mini OS Simulator

## Description

Mini OS Simulator is a C-based project that demonstrates fundamental operating system concepts through an interactive command-line interface. It integrates process management, CPU scheduling, memory allocation, deadlock avoidance, paging, and basic file system operations into a single system.

The project is intended for educational purposes to simulate how an operating system manages processes and resources internally.

---

## Features

### File System Commands
- `ls` – List directory contents  
- `cd <dir>` – Change directory  
- `mkdir <dir>` – Create directory  
- `rm <file>` – Remove file  
- `rmdir <dir>` – Remove directory  
- `cat <file>` – Display file contents  
- `nano <file>` – Open GTK-based text editor  

---

### Process Management
- `run <file>` – Compile and create a process (initially suspended)  
- `ps` – Display process table  

Each process includes:
- Process ID (PID)  
- Name  
- Memory allocation  
- Execution state  

Processes are created using Windows API (`CreateProcess`).

---

### CPU Scheduling (Round Robin)
- `schedule` – Executes Round Robin scheduling  

Characteristics:
- Fixed time quantum  
- Cyclic execution  
- Thread-based scheduling  
- Uses `SuspendThread` and `ResumeThread`  

---

### Memory Management (Simulation)
- Each process is assigned memory (10–110 MB simulated)  
- Memory usage tracked per process  

---

### Deadlock Avoidance (Banker’s Algorithm)

Commands:
- `setmax <idx> <r0> <r1> <r2>`  
- `request <idx> <r0> <r1> <r2>`  
- `release <idx> <r0> <r1> <r2>`  
- `deadlock`  

Features:
- Safe state detection  
- Resource allocation tracking  
- Deadlock prevention  

---

### Paging (FIFO Page Replacement)
- `page` – Demonstrates FIFO paging  

Output includes:
- Page hits  
- Page faults  

---

### GTK Text Editor
- Basic GUI editor for creating `.c` files  
- Save functionality included  

---

## Technologies Used

- C Programming Language  
- Windows API (`CreateProcess`, threading)  
- GTK 4 (GUI editor)  
- Standard C Libraries  

---

## Build Instructions

### Requirements

- GCC (MinGW / MSYS2 recommended)  
- GTK 4 development libraries  
- Windows environment  

---

### Compile

```bash
gcc main.c -o jackfruit.exe `pkg-config --cflags --libs gtk4`