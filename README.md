# Custom C++ POSIX Shell

A lightweight, fully functional POSIX-compliant Unix shell built entirely from scratch in C++. 

This project bypasses standard abstraction libraries (like GNU `readline`) to interface directly with the operating system kernel. It demonstrates a deep practical understanding of core systems programming concepts, including process lifecycle management, inter-process communication (IPC), file descriptor routing, and low-level hardware terminal interrupts.

## 🚀 Features

* **Interactive REPL:** A robust Read-Evaluate-Print-Loop supporting dynamic user input.
* **Raw Terminal Tab-Completion:** Custom implementation of `<TAB>` autocompletion for both `$PATH` binaries and local directories by hijacking the terminal's Canonical mode via `<termios.h>`.
* **Multi-Stage Pipelines (`|`):** Supports chaining an arbitrary number of commands, piping standard output to standard input across isolated memory spaces.
* **I/O Redirection:** Full support for routing standard streams (`>`, `>>`, `<`, `2>`, `2>>`) to and from the local filesystem.
* **Native Execution:** Resolves and executes external system binaries by scanning the `$PATH` environment variable.
* **Built-in Commands:** State-aware implementations of `cd`, `pwd`, `echo`, `type`, and `exit`.

## 🧠 System Architecture & OS Concepts

This shell was built to explore the boundaries between user-space applications and the Linux/Unix kernel.

### 1. Process Management (Fork & Exec)
External commands are executed using the classic Fork-Exec pattern. The shell utilizes `fork()` to clone the parent process and `execv()` to overwrite the child's memory space with the target binary. The parent shell acts as a "Grim Reaper," using `waitpid()` to suspend execution until the child terminates, effectively preventing resource leaks and Zombie processes.

### 2. Inter-Process Communication (IPC)
Pipelines (`ls | grep | wc`) are handled by dynamically generating anonymous OS pipes (`pipe()`). The shell loops through the pipeline, forking a child for each command, and passes a "read-end" file descriptor baton down the chain. This allows entirely isolated processes to stream data to each other in real-time.

### 3. File Descriptor Routing
To implement I/O redirection, the shell manipulates the POSIX standard streams (`STDIN_FILENO`, `STDOUT_FILENO`, `STDERR_FILENO`). By utilizing `open()` with specific bitwise flags (`O_WRONLY | O_CREAT | O_TRUNC | O_APPEND`) and `dup2()`, the shell temporarily hijacks the standard output of a process and writes it directly to disk before restoring the terminal state.

### 4. Raw TTY Manipulation
Instead of relying on standard `std::getline`, this shell takes manual control of the terminal emulator. By utilizing `<termios.h>`, the shell disables the `ECHO` and `ICANON` flags, switching the terminal into "Raw Mode." This allows the shell to intercept byte-by-byte hardware interrupts (like the `Tab` key or `Backspace`) to implement a custom, prefix-matching autocompletion engine from scratch.

## 🛠️ Installation & Usage

### Prerequisites
* A Unix-like operating system (Linux, macOS, or WSL).
* A modern C++ compiler (`g++` or `clang++`) supporting C++11 or higher.

### Build Instructions
Compile the monolithic source file into an executable binary:
```bash
g++ -O3 -std=c++17 main.cpp -o custom_shell
```
Run
Launch the interactive shell:

```bash
./custom_shell
Example Usage
Bash
$ pwd
/home/user/workspace
$echo "Hello World" > output.txt$ cat < output.txt | wc -w
2
$ t<TAB><TAB>
tail  tar  tee  test  touch  tr  true  tsort  tty  type
$ty<TAB>$ type exit
exit is a shell builtin
```

👨‍💻 Author
Gautam Kumar

Built as a capstone exploration into Operating Systems, C++ systems programming, and POSIX API standards.