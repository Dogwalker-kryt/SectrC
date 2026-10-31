# SectrC

SectrC is a lightweight, command-line-focused reimplementation of [Sectr-ctl](https://github.com/Dogwalker-kryt/Sectr-ctl). It is written primarily in C, with some C++ used where it is currently convenient. The aim is to keep the project smaller and more direct than the original, emphasizing terminal use rather than a graphical interface or interactive TUI.

Sectr-ctl is a Linux storage-management project intended to make disk workflows safer through validation, confirmations, dry-run support, and logging. SectrC is a separate, actively developing implementation—not a drop-in replacement—and is not yet feature-equivalent to Sectr-ctl.

> **Development status:** SectrC is at an early stage. The current program provides command-line argument handling and basic log operations; the storage-management capabilities described by the upstream project are not all implemented here yet. Check the source and `--help` output for what the current build supports.

## Current commands

```text
./ox --help, -h          Show usage and available options
./ox --no-color, -nc     Disable colored output
./ox --view-logs, -vl [N]  Display all log messages, or the last N lines
./ox --clear-logs, -cl   Clear the log file
./ox --version, -v       Print the version field (currently a placeholder)
```

The parser currently accepts one or more recognized flags. Although some planned flags may appear in the general usage text, only use options listed by the current program's `--help` output.

For example, `./ox --view-logs 20` shows the last 20 lines, while `./ox --view-logs` shows the full log.

## Requirements

- Linux or another compatible POSIX environment
- GNU Make
- `g++` with support for the project's C/C++ sources
- OpenSSL development files, including `libcrypto`

On Debian or Ubuntu, the build dependencies can be installed with:

```sh
sudo apt install build-essential libssl-dev
```

## Build

From the repository root directory, build with:

```sh
make
```

The Makefile builds the `ox` executable. To run it:

```sh
./ox --help
```

A debug build is available with `make debug`; it produces `ox_debug`. Remove build outputs with `make clean`.

## Logs

The program uses a `log.txt` file beside the executable for logging. The `--view-logs` and `--clear-logs` options operate on that log. The current startup path initializes logging before parsing command-line options, so ensure the program can create or access its log file in the executable's directory.

## Safety

SectrC is under active development and should not be relied on for critical data. Storage-management features, as they are added, may perform destructive operations. Back up important data, verify device paths, and understand each operation before running it. Do not run unfinished or destructive functionality on disks containing data you need.

## Relationship to Sectr-ctl

SectrC is an independent rewrite of [Sectr-ctl](https://github.com/Dogwalker-kryt/Sectr-ctl), created by the same author. It shares the project's goals and is intended to provide a smaller, mostly-C command-line implementation, but its source was written again for SectrC; it does not contain code copied over from Sectr-ctl. SectrC is not a fork or a drop-in replacement. Features, options, configuration formats, and behavior may differ between the projects.

The projects are licensed separately: Sectr-ctl is licensed under GPL-3.0, while SectrC is licensed under Apache-2.0 as stated below and in [LICENSE](LICENSE). Sectr-ctl's license applies to Sectr-ctl; SectrC's license applies to this repository.

## License

SectrC is distributed under the terms of the Apache License 2.0; see [LICENSE](LICENSE).
