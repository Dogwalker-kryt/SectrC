#!/usr/bin/env bash
#
# SectrC setup: installs dependencies, builds, and installs `sectr` for the current user.
#
#   ./setup.sh               install dependencies, build, install to ~/.local/bin
#   ./setup.sh --skip-deps   build and install only
#   ./setup.sh --uninstall   remove ~/.local/bin/sectr (keeps logs)
#   ./setup.sh --purge       remove ~/.local/bin/sectr and all logs
#
set -euo pipefail

cd "$(dirname "$(readlink -f "$0")")"

BIN_NAME="sectr"
BIN_DIR="$HOME/.local/bin"
if [[ "${XDG_STATE_HOME:-}" == /* ]]; then STATE_DIR="$XDG_STATE_HOME/sectrc"; else STATE_DIR="$HOME/.local/state/sectrc"; fi

# ───────────────────────────── styling ─────────────────────────────
if [[ -t 1 && -z "${NO_COLOR:-}" ]]; then
    B=$'\e[1m'; D=$'\e[2m'; I=$'\e[3m'; R=$'\e[0m'
    RED=$'\e[38;5;203m'; GRN=$'\e[38;5;114m'; YLW=$'\e[38;5;221m'
    BLU=$'\e[38;5;75m';  MAG=$'\e[38;5;177m'; CYN=$'\e[38;5;80m'; GRY=$'\e[38;5;245m'
    G1=$'\e[38;5;51m'; G2=$'\e[38;5;45m'; G3=$'\e[38;5;39m'; G4=$'\e[38;5;63m'; G5=$'\e[38;5;99m'; G6=$'\e[38;5;135m'
    FANCY=1
else
    B=; D=; I=; R=; RED=; GRN=; YLW=; BLU=; MAG=; CYN=; GRY=; G1=; G2=; G3=; G4=; G5=; G6=
    FANCY=0
fi

OK="${GRN}${B}✔${R}"; FAIL="${RED}${B}✘${R}"; WARN="${YLW}${B}!${R}"; ARROW="${CYN}${B}➜${R}"

banner() {
    echo
    echo "  ${G1}${B}███████╗███████╗ ██████╗████████╗██████╗  ██████╗${R}"
    echo "  ${G2}${B}██╔════╝██╔════╝██╔════╝╚══██╔══╝██╔══██╗██╔════╝${R}"
    echo "  ${G3}${B}███████╗█████╗  ██║        ██║   ██████╔╝██║     ${R}"
    echo "  ${G4}${B}╚════██║██╔══╝  ██║        ██║   ██╔══██╗██║     ${R}"
    echo "  ${G5}${B}███████║███████╗╚██████╗   ██║   ██║  ██║╚██████╗${R}"
    echo "  ${G6}${B}╚══════╝╚══════╝ ╚═════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝${R}"
    echo
    echo "  ${D}${I}Linux disk tool  ·  setup${R}  ${GRY}${VERSION}${R}"
    echo "  ${GRY}──────────────────────────────────────────────────${R}"
}

STEP=0; STEPS=4
step() { STEP=$((STEP + 1)); echo; echo "  ${MAG}${B}[$STEP/$STEPS]${R} ${B}$1${R}"; }
info() { echo "        ${GRY}$1${R}"; }
ok()   { echo "    $OK  $1"; }
warn() { echo "    $WARN  $1"; }
die()  { echo; echo "    $FAIL  ${RED}${B}$1${R}"; [[ -n "${2:-}" ]] && echo "        ${GRY}$2${R}"; echo; exit 1; }

LOG_FILE=$(mktemp -t sectrc-setup.XXXXXX)
trap 'rm -f "$LOG_FILE"; [[ $FANCY == 1 ]] && printf "\e[?25h"' EXIT

# run "message" cmd...  -> runs cmd with a spinner, output goes to $LOG_FILE
run() {
    local msg=$1; shift
    if [[ $FANCY == 0 ]]; then
        echo "    ... $msg"
        "$@" >>"$LOG_FILE" 2>&1 || { tail -n 25 "$LOG_FILE" | sed 's/^/        /'; die "$msg failed"; }
        ok "$msg"; return
    fi

    "$@" >>"$LOG_FILE" 2>&1 &
    local pid=$! i=0 start=$SECONDS
    local frames=(⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏)
    printf '\e[?25l'
    while kill -0 "$pid" 2>/dev/null; do
        printf "\r    ${CYN}${B}%s${R}  %s ${GRY}%ds${R}  " "${frames[i++ % ${#frames[@]}]}" "$msg" $((SECONDS - start))
        sleep 0.08
    done
    printf '\e[?25h'
    if wait "$pid"; then
        printf "\r\e[K    %s  %s ${GRY}%ds${R}\n" "$OK" "$msg" $((SECONDS - start))
    else
        printf "\r\e[K    %s  ${RED}${B}%s${R}\n\n" "$FAIL" "$msg"
        echo "        ${B}Last output:${R}"
        tail -n 25 "$LOG_FILE" | sed "s/^/        ${GRY}│${R} /"
        die "$msg failed"
    fi
}

usage() {
    sed -n '3,9p' "$0" | sed 's/^# \{0,1\}//'
    exit 0
}

# ───────────────────────────── arguments ─────────────────────────────
SKIP_DEPS=0; MODE=install
for arg in "$@"; do
    case "$arg" in
        --skip-deps) SKIP_DEPS=1 ;;
        --uninstall) MODE=uninstall ;;
        --purge)     MODE=purge ;;
        -h|--help)   usage ;;
        *) die "Unknown option: $arg" "Run ./setup.sh --help" ;;
    esac
done

VERSION=$(git describe --tags --always --dirty 2>/dev/null || echo development)
banner

# ───────────────────────────── uninstall ─────────────────────────────
if [[ $MODE != install ]]; then
    STEPS=1; step "Uninstalling"
    if [[ -e "$BIN_DIR/$BIN_NAME" ]]; then rm -f "$BIN_DIR/$BIN_NAME"; ok "Removed ${B}$BIN_DIR/$BIN_NAME${R}"; else info "$BIN_DIR/$BIN_NAME is not installed"; fi
    if [[ $MODE == purge ]]; then
        rm -rf "$STATE_DIR"; ok "Removed logs in ${B}$STATE_DIR${R}"
    else
        info "Logs kept in $STATE_DIR (use --purge to remove them)"
    fi
    echo; echo "  ${GRN}${B}Done.${R}"; echo
    exit 0
fi

# ───────────────────────────── 1. system check ─────────────────────────────
step "Checking your system"

[[ "$(uname -s)" == Linux ]] || die "SectrC only runs on Linux" "Detected: $(uname -s)"
[[ $EUID -ne 0 ]] || warn "Running as root: ${B}sectr${R} will be installed for root only ($BIN_DIR)"
[[ -f Makefile && -d src ]] || die "Run this script from the SectrC repository" "Makefile or src/ not found in $(pwd)"

DISTRO="unknown"
[[ -r /etc/os-release ]] && DISTRO=$(. /etc/os-release && echo "${PRETTY_NAME:-$ID}")
ok "System    ${B}$DISTRO${R} ${GRY}($(uname -m))${R}"

PM=""
for candidate in apt-get dnf pacman zypper; do
    if command -v "$candidate" >/dev/null 2>&1; then PM=$candidate; break; fi
done
if [[ -n $PM ]]; then ok "Packages  ${B}$PM${R}"; else warn "No supported package manager found"; fi

# ───────────────────────────── 2. dependencies ─────────────────────────────
step "Installing dependencies"

have_deps() {
    command -v gcc >/dev/null && command -v g++ >/dev/null && command -v make >/dev/null &&
    command -v pkg-config >/dev/null && pkg-config --exists libcrypto libudev blkid
}

if [[ $SKIP_DEPS == 1 ]]; then
    info "Skipped (--skip-deps)"
elif have_deps; then
    ok "gcc, g++, make, OpenSSL, libudev, libblkid already installed"
else
    case "$PM" in
        apt-get) PKGS=(build-essential pkg-config libssl-dev libudev-dev libblkid-dev)
                 INSTALL=(apt-get install -y "${PKGS[@]}"); REFRESH=(apt-get update) ;;
        dnf)     PKGS=(gcc gcc-c++ make pkgconf-pkg-config openssl-devel systemd-devel libblkid-devel)
                 INSTALL=(dnf install -y "${PKGS[@]}"); REFRESH=() ;;
        pacman)  PKGS=(base-devel pkgconf openssl systemd-libs util-linux-libs)
                 INSTALL=(pacman -S --needed --noconfirm "${PKGS[@]}"); REFRESH=() ;;
        zypper)  PKGS=(gcc gcc-c++ make pkg-config libopenssl-devel libudev-devel libblkid-devel)
                 INSTALL=(zypper --non-interactive install "${PKGS[@]}"); REFRESH=() ;;
        *) die "Cannot install dependencies automatically" "Install gcc, g++, make, pkg-config and the OpenSSL, libudev and libblkid dev packages, then run ./setup.sh --skip-deps" ;;
    esac

    info "Packages: ${PKGS[*]}"
    SUDO=()
    if [[ $EUID -ne 0 ]]; then
        command -v sudo >/dev/null || die "sudo is required to install packages" "Or run with --skip-deps after installing them yourself"
        echo "        ${YLW}${B}sudo${R}${YLW} needs your password to install packages:${R}"
        sudo -v || die "sudo authentication failed"
        SUDO=(sudo)
    fi
    [[ ${#REFRESH[@]} -gt 0 ]] && run "Refreshing package lists" "${SUDO[@]}" "${REFRESH[@]}"
    run "Installing ${#PKGS[@]} packages with $PM" "${SUDO[@]}" "${INSTALL[@]}"
    have_deps || die "Dependencies still missing after install" "Check the packages above and run ./setup.sh --skip-deps"
fi

# ───────────────────────────── 3. build ─────────────────────────────
step "Building SectrC"

JOBS=$(nproc 2>/dev/null || echo 2)
run "Cleaning old build"   make clean
run "Compiling and linking ${GRY}($JOBS jobs)${R}" make -j"$JOBS"
[[ -x ./ox ]] || die "Build finished but ./ox was not produced"
ok "Binary    ${B}$(du -h ox | cut -f1)${R}  ${GRY}$(./ox --no-color --version)${R}"

# ───────────────────────────── 4. install ─────────────────────────────
step "Installing"

mkdir -p "$BIN_DIR" "$STATE_DIR"
chmod 700 "$STATE_DIR"
install -m 755 ox "$BIN_DIR/$BIN_NAME"
ok "Installed ${B}${BIN_DIR/#$HOME/\~}/$BIN_NAME${R}"
ok "Log file  ${B}${STATE_DIR/#$HOME/\~}/log.txt${R}"

PATH_OK=1
case ":$PATH:" in *":$BIN_DIR:"*) ;; *) PATH_OK=0 ;; esac

# ───────────────────────────── summary ─────────────────────────────
# Pads by visible characters (printf %-Ns counts bytes, which breaks on ✔ and ~ paths).
pad() { local n=$(( $2 - ${#1} )); (( n < 0 )) && n=0; printf '%s%*s' "$1" "$n" ''; }
row() { echo "  ${GRN}│${R}  ${GRY}$(pad "$1" 10)${R}${B}$(pad "$2" 36)${R}  ${GRN}│${R}"; }
echo
echo "  ${GRN}╭──────────────────────────────────────────────────╮${R}"
echo "  ${GRN}│${R}  ${GRN}${B}$(pad "✔  SectrC is installed!" 46)${R}  ${GRN}│${R}"
echo "  ${GRN}├──────────────────────────────────────────────────┤${R}"
row "version" "$VERSION"
row "binary"  "${BIN_DIR/#$HOME/\~}/$BIN_NAME"
row "logs"    "${STATE_DIR/#$HOME/\~}/log.txt"
echo "  ${GRN}╰──────────────────────────────────────────────────╯${R}"
echo

if [[ $PATH_OK == 0 ]]; then
    warn "${B}${BIN_DIR/#$HOME/\~}${R} is not in your PATH. Add it with:"
    case "${SHELL##*/}" in
        fish) echo "        ${CYN}fish_add_path ~/.local/bin${R}" ;;
        zsh)  echo "        ${CYN}echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.zshrc && source ~/.zshrc${R}" ;;
        *)    echo "        ${CYN}echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc && source ~/.bashrc${R}" ;;
    esac
    echo
fi

echo "  ${ARROW} Try it:  ${B}${CYN}$BIN_NAME --help${R}"
echo "  ${ARROW} Disks:   ${B}${CYN}$BIN_NAME --list${R}"
echo
