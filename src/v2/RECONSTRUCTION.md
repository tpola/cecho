# cecho v2.0 reconstruction notes

This is **not a byte-for-byte recovery** of the lost `cecho_src.zip`.

It reconstructs the August 2010 v2.0 described by Thomas Polaert's archived
CodeProject article **“Add Colors to Batch Files”**.

## Directly supported by the article

The article prints:

- the `Grammar`, `Action`, and `Successor` state-machine tables;
- the `tokStack` parser model;
- the `wmain` parsing loop;
- use of `GetCommandLineW()` to preserve spaces;
- `setColor()` based on `SetConsoleTextAttribute`;
- syntax `{XX}`, `{color}`, `{\n}`, `{\t}`, `{\uXXXX}`, `{{`, `{#}`;
- v2.0 history: **Unicode support** and **x64 support** added 23 Aug 2010.

## Reconstructed from behavior + later v3 source

The article says the action functions are “quite simple” and does not print
them all. Therefore these functions are reconstructed:

- `printStack`
- `resetStack`
- `printEscChar`
- `printHexChar`
- `parseColor`
- initial console color capture for `{#}`

## Intentionally removed from the later v3 source

These appear in the recovered v3.00 but are not documented as v2.0 features:

- `-n`, `-r`, `-z`, `-h`, `-v`
- automatic trailing newline
- redirected stdout detection
- UTF-16 → UTF-8 conversion for pipes/files
- enhanced parser error reporting
- `ntdll.h`
- `"default"` named-color alias
- additional v3 parser state/transitions

## Two small compile-oriented corrections

The archived article's snippets appear to contain two likely transcription or
presentation issues:

1. `setColor()` is shown calling `strtol(tokStack, ...)` even though `tokStack`
   is `wchar_t[]`. This reconstruction uses `wcstol`.
2. the syntax-error column expression is printed as `input - token`, which
   yields a negative offset. This reconstruction uses `token - input`.

Both differences are documented inline in `cecho_v2_reconstructed.c`.