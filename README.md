# keep

## 📝 Overview

- **Keep** is a C programming language-based VCS(Version Control System) designed by professor in an OSS(Open Source Software) class.
- The program is designed for <ins>educational purposes</ins> to make it easier for students to understand and learn about the VCS.
- It is only <ins>conceptually designed</ins> without any substantive implementation, and it has been provided with basic designs and guidelines for students to implement directly through the task.

## 📋 Guidelines

### Commands

| Instruction | Description |
|:-|:-|
| `keep init` | Creates a backup space in the current directory and initializes it. |
| `keep track <file or directory>` | Tracks the specified file `<file>` or all files in the specified directory `<directory>`.  |
| `keep store "<note>"` | Stores the current state of the directory and leaves a message with the specified `<note>`.  |
| `keep restore "<version>"` | Restores the state of the directory to the specified version `<version>`. |
| `keep versions` | Displays a list of all versions. |

## 🛠️ Installation

1. Visit the [Releases](https://github.com/seungjoonH/keep/releases) page.
2. Download the latest version (`keep 1.0.0.zip`).
3. Extract the ZIP file.

## ▶️ Execution

1. Make execution file

```bash
$ make
```

2. Set alias.

```bash
$ alias keep="$PWD/keep"
```

3. Initialize by using keywords `keep`

```bash
$ keep init
```

4. Use the commands below to manage versions

```bash
$ keep track
$ keep store
$ keep restore
```

5. List the versions

```bash
$ keep versions
```

## ⚙️ Requirements

1. **Make**: A build automation tool that helps compile and manage the project.  
   
Install make via Homebrew:

```bash
$ brew install make
```

2. **GCC (GNU Compiler Collection)**: A compiler for the C programming language.  

Install GCC via Homebrew:

```bash
$ brew install gcc
```

## 📚 Articles

- Related [Blog Post](https://seungjoonh.tistory.com/entry/project-keep)
