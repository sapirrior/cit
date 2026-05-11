<p align="center">
  <img src="assets/cit-round.png" alt="Cit Logo" width="100">
</p>

# Cit - Local-First Version Control

Cit is a high-integrity version control system written in C. It provides a secure and verifiable local version control experience by employing SHA-256 content addressing, hierarchical Merkle trees, and advanced storage optimizations.

## Core Technical Specifications

- **Data Integrity**: Mandatory SHA-256 identification for all objects (blobs, trees, and commits).
- **Architecture**: Hierarchical Merkle tree structure for efficient directory-level hashing and state management. Unlike flat-index systems, Cit trees are nested objects that mirrors the filesystem hierarchy.
- **Storage Optimization**: 
    - **Cit-LZ Compression**: Proprietary hybrid LZSS + Range Coder engine for high-ratio data compression.
    - **Recursive Delta Encoding**: Binary delta support for blobs, significantly reducing the storage footprint for versioned files with incremental changes.
- **Security**: Centralized path normalization and transactional object creation with automatic rollback on failure to maintain repository consistency.
- **Portability**: POSIX-compliant implementation supporting Linux, macOS, Android (Termux), and Windows (via compatibility wrappers).

## Installation and System Integration

### 1. Build from Source
To compile Cit, ensure that the GCC compiler, Make, and the zlib development library are installed on your system. 

```bash
make
```
The compiled binary will be located in the `build/` directory.

### 2. Global Installation
To use Cit from any directory, move the compiled binary to a location in your system's execution path.

For Linux, macOS, or Termux:
```bash
# Move the binary to a standard local bin directory
mkdir -p ~/bin
cp build/cit ~/bin/

# Ensure ~/bin is in your shell's PATH
export PATH="$HOME/bin:$PATH"
```

## Initial Configuration
Before performing commit operations, configure your global identity:
```bash
cit config -u "Full Name"
cit config -e "email@example.com"
```

## CLI Reference

### Global Flags
- `-h, --help`: Show help message.
- `-v, --version`: Show version information from `src/version.txt`.

### Repository Management
- `cit init`: Initialize a new Cit repository in the current directory.
- `cit clone <url>`: Clone an external repository (libcurl-powered).

### Staging and Status
- `cit add <file>`: Stage file contents for the next commit.
- `cit status`: Display the working tree status and staged changes.

### History and Commits
- `cit commit <message>`: Record a new snapshot of staged changes.
- `cit log`: View commit history with SHA-256 identifiers.
- `cit show <sha>`: Display detailed information about a specific object.

### Branching and Checkout
- `cit branch`: List, create, or delete branches.
- `cit checkout <branch|sha>`: Switch branches or restore working tree files via recursive tree traversal.
- `cit diff`: Show changes between commits, commit and working tree, or staged and unstaged files.

### Recovery
- `cit reset`: Reset current HEAD to a specified state.

## Architecture Overview
The Cit core is modularized into several key components:
- **`core/object`**: Manages the lifecycle of blobs, hierarchical trees, and commits.
- **`core/compress`**: Implements the Cit-LZ compression and delta encoding logic.
- **`core/index`**: Manages the staging area with strict path normalization.
- **`core/refs`**: Handles branch pointers and HEAD resolution.

## License
This project is licensed under the MIT License.
