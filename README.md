# 🔒 SSL Certificate Downloader

A multi-threaded SSL certificate downloader that allows you to retrieve and save SSL certificates from multiple hosts.

## 📋 Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Required Libraries](#required-libraries)
- [Compilation](#compilation)
- [Usage](#usage)
- [Arguments](#arguments)
- [Examples](#examples)
- [Contributing](#contributing)
- [License](#license)

## ✨ Features

- 🚀 Multi-threaded certificate downloading
- 💾 Save certificates with SHA256 hash filenames
- ⏱️ Configurable connection timeout
- 🔄 Optional overwrite of existing certificates
- 🕰️ Customizable delay between requests

## 🛠️ Requirements

- GCC compiler
- OpenSSL development libraries
- POSIX-compliant system

## 📚 Required Libraries

Before compiling, ensure you have the following libraries installed on your system:

- 🔐 OpenSSL: For SSL/TLS support
- 🧵 pthreads: For multi-threading capabilities

These libraries are typically available in most standard development environments. If you encounter any issues during compilation, make sure these libraries and their development files are properly installed on your system.

## 🔨 Compilation

1. Clone the repository:
```
git clone https://github.com/TheScriptGuy/certificate-downloader.git
cd ssl-certificate-downloader
```

2. Compile the program:
Make options
```
$ make help
Available targets:
  all       : Build the default target (download_cert)
  full      : Build with all libraries statically linked
  clean     : Remove all built and intermediate files
  help      : Display this help message

Compiler flags:
  CFLAGS    : -Wall -Wextra -Iinclude -pthread
  LDFLAGS   : -lssl -lcrypto

To use a specific compiler, set CC. For example:
  make CC=clang

To add extra CFLAGS or LDFLAGS, use '+=' For example:
  make CFLAGS+=-DDEBUG LDFLAGS+=-L/usr/local/lib
```

To build the binary with the libraries `dynamically` linked, use:
```
make all
```

To build the binary with the libraries `statically` linked, use:
```
make full
```

This will create an executable named `download_cert`.

## 🚀 Usage

Basic usage:
```
./download_cert -if <input_file> -od <output_directory> [OPTIONS]
```

## 🎛️ Arguments

| Argument | Description | Required |
|----------|-------------|----------|
| `-if <input_file>` | Input file containing hostnames and ports | ✅ Yes |
| `-od <output_directory>` | Directory to save downloaded certificates | ✅ Yes |
| `-delay <seconds>` | Delay between each worker's request (default: 0) | ❌ No |
| `-workers <number>` | Number of worker threads (default: 1) | ❌ No |
| `-timeout <seconds>` | Connection timeout in seconds (default: 3) | ❌ No |
| `-overwrite` | Allow overwriting of existing certificate files | ❌ No |

## 📝 Examples

1. Basic usage with default settings:
```
./download_cert -if hosts.txt -od /path/to/certs
```

2. Use 5 worker threads with a 2-second delay between requests:
```
./download_cert -if hosts.txt -od /path/to/certs -workers 5 -delay 2
```

3. Set a 10-second connection timeout and allow overwriting:
```
./download_cert -if hosts.txt -od /path/to/certs -timeout 10 -overwrite
```

4. Combine multiple options:
```
./download_cert -if hosts.txt -od /path/to/certs -workers 3 -delay 1 -timeout 5 -overwrite
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📄 License

This project/software is licensed for Personal Use only - see the [LICENSE](https://github.com/TheScriptGuy/download-certificates/blob/main/LICENSE.md) file for details.
