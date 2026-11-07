"""Fails the build when the program leaves too little RAM for the software stack.

The linker checks that the static sections fit in RAM, but not the stack. The stack
starts at __stack and grows down, so the room left is __stack minus the end of the
last RAM section. When that room is under the minimum, a deep call corrupts memory
at run time, so the build fails here instead.

Usage: python assets/check_memory.py ELF LLVM_NM LLVM_SIZE MIN_STACK_BYTES
"""

import subprocess
import sys

RAM_START = 0x0200


def stack_top(nm, elf):
    out = subprocess.run([nm, elf], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == "__stack":
            return int(parts[0], 16)
    raise SystemExit("check_memory: __stack is not in %s" % elf)


def static_end(size, elf):
    out = subprocess.run([size, "-A", elf], capture_output=True, text=True, check=True).stdout
    end = 0
    for line in out.splitlines():
        parts = line.split()
        if len(parts) != 3 or not parts[1].isdigit() or not parts[2].isdigit():
            continue
        addr, length = int(parts[2]), int(parts[1])
        if addr >= RAM_START and length > 0:
            end = max(end, addr + length)
    if end == 0:
        raise SystemExit("check_memory: no RAM sections in %s" % elf)
    return end


def main():
    if len(sys.argv) != 5:
        raise SystemExit(__doc__)
    elf, nm, size, minimum = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
    top = stack_top(nm, elf)
    end = static_end(size, elf)
    room = top - end
    print("stack room: %d bytes (static RAM ends at $%04X, stack starts at $%04X)" % (room, end, top))
    if room < minimum:
        raise SystemExit(
            "check_memory: only %d bytes of stack room, at least %d are needed. "
            "Remove code or RAM data, or move assets to disk." % (room, minimum))


if __name__ == "__main__":
    main()
