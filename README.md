# SectrC

[![Build & Release](https://github.com/Dogwalker-kryt/SectrC/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/Dogwalker-kryt/SectrC/actions/workflows/build-and-release.yml)
[![Latest release](https://img.shields.io/github/v/release/Dogwalker-kryt/SectrC?sort=semver)](https://github.com/Dogwalker-kryt/SectrC/releases/latest)
[![License: Apache-2.0](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

**SectrC** is a small, fast Linux command-line tool for inspecting disks and partitions, written in plain C.
It is a lightweight rewrite of [Sectr-ctl](https://github.com/Dogwalker-kryt/Sectr-ctl) that focuses on the terminal: no GUI and no TUI, just one binary called `ox`.

> [!WARNING]
> SectrC is in **early development**. Today it only *reads* disk information. Later versions will add storage-management features that can destroy data.
> Always check device names and keep backups.

---

## Features

| Feature | Command | Status |
|---|---|---|
| List disks and partitions (model, vendor, bus, serial, size, filesystem, UUID, label, mountpoint) | `ox --list` | ✅ |
| Filter the list to one disk or partition | `ox --list sda` | ✅ |
| SHA-256 fingerprint of a drive's metadata | `ox --fingerprint nvme0n1` | ✅ |
| Built-in log viewer and log cleaner | `ox --view-logs`, `ox --clear-logs` | ✅ |
| Storage-management operations (from Sectr-ctl) | — | 🚧 planned |

## Quick start

### Download a release

Prebuilt Linux x86-64 binaries are attached to every [release](https://github.com/Dogwalker-kryt/SectrC/releases):

```sh
tar -xzf ox-v*-linux-x86_64.tar.gz
./ox --help
```

You still need the runtime libraries: `libssl`, `libudev` and `libblkid`. Most desktop distributions already have them.

### Build from source

**Dependencies** (Debian / Ubuntu):

```sh
sudo apt install build-essential libssl-dev libudev-dev libblkid-dev
```

<details>
<summary>Fedora / Arch</summary>

```sh
# Fedora
sudo dnf install gcc gcc-c++ make openssl-devel systemd-devel libblkid-devel
# Arch
sudo pacman -S base-devel openssl systemd-libs util-linux-libs
```
</details>

**Build:**

```sh
git clone https://github.com/Dogwalker-kryt/SectrC.git
cd SectrC
make            # builds ./ox
./ox --version
```

| Make target | What it does |
|---|---|
| `make` | Optimized build → `./ox` |
| `make debug` | Debug build with symbols, `-O0` → `./ox_debug` |
| `make run` / `make run_debug` | Build, then run |
| `make version` | Print the version that will be built into the binary |
| `make clean` | Delete all build output |

## Usage

```text
ox [options]

  --help         -h                Show the help message
  --version      -v                Show the current version
  --no-color     -nc               Disable colored output
  --view-logs    -vl  [lines]      Show the log (all lines, or the last N)
  --clear-logs   -cl               Clear the log file
  --list         -l   [disk-name]  List disks, or only one disk / partition
  --fingerprint  -fp  <disk-name>  Show a SHA-256 fingerprint of a drive
```

Options with a value accept both `--opt value` and `--opt=value`.
You can give device names with or without `/dev/` (`sda1` and `/dev/sda1` both work).

### Examples

```sh
# Every disk and partition
./ox --list

# Only nvme0n1 and its partitions
./ox -l nvme0n1

# Fingerprint a drive
./ox --fingerprint /dev/sda

# Show the last 20 log lines
./ox -vl 20
```

Example output of `ox --list sda1` (made-up values):

```text
Device: /dev/sda1 (partition), 536870912 bytes
  Parent: /dev/sda | Model: Samsung_SSD_870 | Vendor: ATA | Bus: ata
  Filesystem: vfat | Label: - | UUID: 1A2B-3C4D | Mountpoint: /boot/efi
  Serial: S6PXNM0T123456 | Status: available
```

> Some fields, such as filesystem probing on unmounted devices, may need `sudo`.

## Logging

Errors are written to `log.txt` in the **same directory as the `ox` binary**.
The program must be able to create and write that file, or it will not start.
Read it with `ox --view-logs` and clear it with `ox --clear-logs`.

## Versioning

You never change version numbers by hand. Versions come from **git tags**, and CI creates the tags for you.

1. Each push to `master` runs the [Build & Release](.github/workflows/build-and-release.yml) workflow.
2. If the build passes, [`scripts/next-version.sh`](scripts/next-version.sh) looks at the commit messages since the last tag and picks the next [semantic version](https://semver.org):

   | Commit message | Bump | Example |
   |---|---|---|
   | `feat: …` / `feat(list): …` | minor | `v0.3.2` → `v0.4.0` |
   | `feat!: …` or `BREAKING CHANGE:` in the body | major (minor while below `1.0.0`) | `v1.4.0` → `v2.0.0` |
   | anything else (`fix: …`, `refactor: …`, `spinner`, …) | patch | `v0.3.2` → `v0.3.3` |

3. CI creates the tag, builds `ox` with that version built in, and publishes a GitHub Release with the binary and generated release notes.

Add `[no release]` to a commit message to push without releasing.

For local builds, `make` runs `git describe` to get the version, so `ox --version` prints something like:

- `v0.4.0`: built exactly from a release tag
- `v0.4.0-3-g1a2b3c4`: three commits after `v0.4.0`
- `…-dirty`: built with uncommitted changes

Run `git fetch --tags` after a release so your local builds see the new tag.

## Project layout

```text
src/
├── main.c            # entry point and CLI option table
├── arg_parser/       # small table-driven argument parser
├── disk_data/        # disk enumeration (libudev + libblkid + mount table)
├── list_drives/      # --list output
├── fingerprint/      # --fingerprint (OpenSSL SHA-256)
├── Debug/            # error codes, ERR()/PERR() and the logger
├── vector/           # generic growable array
├── read_sys_fs/      # /sys file helpers
├── path/             # executable-relative paths
├── spinner/          # threaded "Running..." spinner
└── sectr_helper/     # log view/clear and misc helpers
scripts/next-version.sh
.github/workflows/build-and-release.yml
```

## Contributing

- Follow [CODE_RULES.md](CODE_RULES.md).
- Use [Conventional Commits](https://www.conventionalcommits.org) (`feat:`, `fix:`, `refactor:`, `docs:`, …) so the version bumps correctly.
- Make sure `make` builds without errors before you push. CI checks every push and pull request.

## Relationship to Sectr-ctl

SectrC is an independent rewrite of [Sectr-ctl](https://github.com/Dogwalker-kryt/Sectr-ctl) by the same author. It shares the goal of safer Linux disk workflows (validation, confirmations, dry runs, logging), but it was written from scratch and is **not** a fork or a drop-in replacement. Options and behavior may differ.

The two projects have different licenses: Sectr-ctl uses **GPL-3.0**, and SectrC uses **Apache-2.0**.

## License

Apache License 2.0. See [LICENSE](LICENSE).
