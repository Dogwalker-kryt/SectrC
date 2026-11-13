# SectrC Code Rules

These rules keep SectrC consistent, easy to review, and lightweight. Follow them for new code and when modifying existing code.

## Language and types

- Prefer C and C-compatible interfaces. Use C++ only where the project currently requires it or where there is a clear reason.
- Use the fixed-width integer types from `<stdint.h>` (for example, `uint64_t`, `uint32_t`, `int32_t`). Do not use `__`-prefixed types such as `__uint64_t`: names starting with `__` are reserved for the compiler and the C library.
- Use `size_t` for sizes and counts of things in memory: buffer sizes, string lengths, array lengths and indexes, `sizeof` results. Use `ssize_t` where a library function returns it (for example, `readlink()`).
- Use `uint64_t` for values that are not about memory and can exceed 4 GiB on a 32-bit system: disk sizes in bytes, numbers read from sysfs, line counts of files.
- Use the narrowest project integer type that safely represents the value. Do not use a signed type for values that can be negative only if the operation actually requires signed values.

## Formatting

- Keep each `if` statement on one line. Do not wrap its condition or split the statement across lines.
- Keep function declarations and definitions on one line, including their parameter lists. Do not wrap parameter lists across multiple lines.
- Match the surrounding file's indentation and brace style when editing existing code.

## Input checks and defensive code

- Validate values that can vary at runtime at the point where they enter the program, unless the caller has already validated them and that guarantee is clear.
- Do not add redundant checks for values that are known at compile time or are guaranteed by the caller. For example, a call such as `strlen("constant")` does not need a preceding null check for that literal.
- Apply the same rule to loops: check values that may change or are not already validated before use; avoid repeating checks when a prior validation guarantees safety.
- Keep checks proportional to the risk. Validate external input, file operations, allocations, and other failure-prone boundaries; avoid unnecessary checks on trusted, fixed values.

## General

- Prefer straightforward, small implementations over unnecessary abstractions or defensive scaffolding.
- Make error handling explicit and consistent with the existing error-reporting utilities.
- Do not silently ignore failures from operations that can affect data or program correctness.