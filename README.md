# CloseReview

A peer-to-peer tool for developers to connect, share files, and review each other's code.

## Table of Contents

    •	Introduction
    •	Prerequisites
    •	Installation
    •	1. Install MariaDB
    •	2. Install Zig
    •	3. Install OpenSSL

## Introduction

CloseReview is a peer-to-peer application designed for developers to effortlessly connect and collaboratively review code. It leverages the power of C for performance, Zig for the build System, MariaDB for database management, and OpenSSL for secure communications.

## Prerequisites

Before installing CloseReview, ensure that you have the following installed on your system:
• MariaDB: An open-source relational database.
• Zig: A modern programming language used for building CloseReview.
• OpenSSL: A robust toolkit for secure communications.

## Installation

Follow the steps below to install CloseReview and its dependencies.

### Install MariaDB

<!-- TODO: Provide MariaDB installation steps -->

### Install Zig

#### Windows

<!-- TODO: Provide Zig installation steps for Windows -->

#### macOS

1. **Install Zig using Homebrew:**

   ```bash
   brew install zig
   ```

2. **Verify the Installation:**

   ```bash
   zig version
   ```

#### Linux

To install Zig on Linux, you can download the latest precompiled binary directly from the official website.
Before trying this however make sure your distribution's package manager doesn't already have the latest version of Zig available :

https://github.com/ziglang/zig/wiki/Install-Zig-from-a-Package-Manager

1. **Download the Latest Release:**

   Visit the [Zig Download Page](https://ziglang.org/download/) and download the appropriate binary for your system architecture. For most Linux users, this will be one of:

2. **Extract the Archive:**

   Open a terminal, navigate to the directory where you downloaded the file, and extract it using the following command:

   ```bash
   tar -xf zig-linux-*-<version>.tar.xz
   ```

   This will create a directory named `zig-linux-*-<version>`.

3. **Move Zig to a Directory in Your PATH:**

   Move the extracted Zig directory to `/opt` (or any location you prefer):

   ```bash
   sudo mv zig-linux-*-<version> /opt/zig
   ```

   Create a symbolic link to make Zig accessible system-wide:

   ```bash
   sudo ln -s /opt/zig/zig /usr/local/bin/zig
   ```

4. **Verify the Installation:**

   Confirm that Zig is installed correctly by checking its version:

   ```bash
   zig version
   ```

   You should see the version number printed in the terminal.

##### Notes:

- **Ensure Executable Permissions:**

  If you encounter a permission error, make sure the Zig binary has the correct executable permissions:

  ```bash
  sudo chmod +x /opt/zig/zig
  ```

- **Updating Zig:**

  To update Zig in the future, download the newer version from the [official website](https://ziglang.org/download/) and replace the files in `/opt/zig`.

### Install OpenSSL

<!-- TODO: Provide OpenSSL installation steps -->
