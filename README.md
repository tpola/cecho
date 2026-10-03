# cecho — Add Colors to Batch Files

Archive of **“Add Colors to Batch Files”**, a CodeProject article by **Thomas Polaert** describing `cecho`, an enhanced `ECHO` command-line utility with color support.

> **Archive status** — The original CodeProject site is no longer serving this historical article. This repository preserves the archived article HTML, a documented reconstruction of **cecho v2.0**, and a later recovered **cecho v3.00** derivative for historical comparison.

## Provenance

- **Author:** Thomas Polaert
- **Original CodeProject article ID:** `17033`
- **Historical URL:** `http://www.codeproject.com/Articles/17033/Add-Colors-to-Batch-Files`
- **Original publication:** January 2007
- **cecho v2.0 article revision:** 23 August 2010
- **Original license:** Code Project Open License (**CPOL**)

### `src/v2/cecho_v2_reconstructed.c`

The original v2 source archive has not been recovered. This file is a **reconstruction**, based on:

1. the C/C++ source fragments printed directly in the archived CodeProject article;
2. the documented v2.0 syntax and behavior;
3. comparison with a later recovered v3.00 derivative.

The reconstruction deliberately removes features that appear to be later v3 additions, including the `-n`, `-r`, `-z`, `-h`, and `-v` command-line switches, redirected-output UTF-8 conversion, the `ntdll.h` dependency, and later parser/error-handling changes. See `src/v2/RECONSTRUCTION.md`.

### `src/v3/cecho.c`

This is a later recovered **cecho v3.00** derivative. Its header credits **Thomas Polaert**, **anzz1**, and **yungDoom**. It is preserved here as a historical descendant of the CodeProject source, **not** as the original 2010 v2.0 source.

## Repository layout

```text
src/v2/                    reconstructed cecho v2.0
src/v3/                    later recovered cecho v3.00 derivative
archive/                   archived CodeProject HTML
```

## Article

The text below is converted from the archived CodeProject HTML so that the article remains readable directly from GitHub. Formatting may differ slightly from the original page; the HTML snapshot in `archive/` is the primary preserved artifact.

---

- [Download source - 3.63 KB](https://web.archive.org/web/20220309120614/https://www.codeproject.com/KB/dialog/BatchFileColors/cecho_src.zip)
- [Download exe - 54.7 KB](https://web.archive.org/web/20220309120614/https://www.codeproject.com/KB/dialog/BatchFileColors/cecho_bin.zip)

![Colorize Batch File](./Add Colors to Batch Files - CodeProject_files/cecho1.png)

## Introduction

Batch files are useful to deal with repetitive tasks. However, they lack some user-friendly features such as colorizing console outputs. Colorized outputs might be useful to draw attention to important information. The Win32 API provides some useful functions to interact with the console (see [Console Functions](https://web.archive.org/web/20220309120614/http://msdn.microsoft.com/en-us/library/ms682073.aspx) in MSDN). But in your batch files, the only command available is `COLOR`. The `COLOR` command only defines the color of the entire window console.

cecho is an enhanced ECHO command line utility with color support, inspired by the [CTEXT utility by Dennis Bareis](https://web.archive.org/web/20220309120614/http://www.dennisbareis.com/).

The last section explains how to embed the cecho utility into a batch file using the *Debug.exe* program (until Windows Vista).

## Using the Code

cecho simply redirects the command arguments to the standard output after parsing color information. cecho arguments include:

- `{XX}`: colors coded as two hexadecimal digits. E.g., {0A} light green
- `{color}`: color information as understandable text. E.g., {light red on black}
- `{\n \t}`: New line character - Tab character.
- `{\u0000}`: Unicode character code.
- `{{`: escape character '{'.
- `{#}`: restore initial colors.

Available colors:

|  |  |
| --- | --- |
| 0 = black | 8 = gray |
| 1 = navy | 9 = blue |
| 2 = green | A = lime |
| 3 = teal | B = aqua |
| 4 = maroon | C = red |
| 5 = purple | D = fuchsia |
| 6 = olive | E = yellow |
| 7 = silver | F = white |

Sample batch file:

```
@echo off

cecho {0C}This line is red{#}

REM Print ASCII char 0x07 (beep) 
cecho {\u07 \u07}

cecho This {black on blue}word{#} is black on a blue background

...
```

Changes log: in cecho v2.0, the {0x00} ASCII code has been replaced by the {\u0000} Unicode character.

## Inside cecho

### Colorized Console Output

Colorizing console output is pretty straightforward. Simply call the `SetConsoleTextAttribute` Win32 API with the correct color code. All further console outputs will be displayed in the specified color. We'll explain `tokStack` later.

C++

```
void setColor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    tokStack[tokCurs-1] = 0;
    SetConsoleTextAttribute(hConsole, (WORD)strtol(tokStack, 0, 16));
}
```

### Parsing Command Arguments

According to the cecho documentation, parsing command arguments can be described by the following parse tree:

![Parse Tree](./Add Colors to Batch Files - CodeProject_files/cecho2.png)

Fig. 1: Parse Tree

The parse tree grammar in C language becomes:

C++

```
#define MAX_TRANS 5

// expected tokens list
wchar_t* Grammar[][MAX_TRANS] = {
    {L"{", L""},
    {L"{}", L"\\", L"0123456789abcdefABCDEF", L" \t", L""},
    {L"tn", L"u"},
    {L"0123456789abcdefABCDEF"},
    {L"0123456789abcdefABCDEF", L" \t", L"}", L"\\"},
    {L"0123456789abcdefABCDEF", L""},
    {L"}", L""},
    {L"}", L""}
};

// action executed on state changed
Fct Action[][MAX_TRANS] = {
    {&printStack,0},
    {&resetStack,0,0,0,0},
    {&printEscChar,0},
    {0},
    {0,&printHexChar,&printHexChar,&printHexChar},
    {0,0},
    {&setColor,0},
    {&parseColor,0}
};

// next state Ids
char Successor[][MAX_TRANS] = {
    {1,0},
    {0,2,5,1,7},
    {1,3},
    {4},
    {4,1,0,2},
    {6,7},
    {0,7},
    {0,7}
};
```

The parser state chart contains 7 states. The `Grammar` array stores lists of expected tokens that trigger a transition for a specific state. An empty list acts as the wild char token. The `Action` array maintains pointers to the function to execute during a specific transition. The `Successor` array identifies the next state for each transition.

The parser engine iterates through all input characters. If the character triggers an outgoing transition of the current parser state, then the character is added to the `tokStack` token stack, the action associated with the transition (if any) is executed, and the parser state is updated.

C++

```
// token stack
wchar_t tokStack[MAX_STACK];
// token stack cursor
int tokCurs = 0;

int wmain(int argc, wchar_t* argv[])
{
    short trans, clr, fired;
    wchar_t* token;
    char state = 0;

    [...]

    // retrieve command argument
    LPWSTR input = GetCommandLineW();
    
    if (*input == L'"')
        input += 2;

    input += wcslen(argv[0]) + 1;

    // parse input string
    for (fired = 0, token = input; *token != 0; token++)
    {
        // does token trigger a transition ?
        for (trans = 0; trans < MAX_TRANS && Grammar[state][trans] != 0; trans++)
        {
            // check if token belongs to the expected tokens list ? 
            // a empty list acts as a wildchar token
            if (wcschr(Grammar[state][trans], *token) || *Grammar[state][trans] == 0)
            {
                // push token into the stack
                if (tokCurs < MAX_STACK)
                    tokStack[tokCurs++] = *token;

                // execute the action associated to the transition (if any)
                if (Action[state][trans] != 0)
                {
                    (*Action[state][trans])();
                    tokCurs = 0; // reset token stack
                }
                // update parser state
                state = Successor[state][trans];
                fired = 1;
                break;
            }
        }
        if (fired == 0)
        {
            wprintf(L"Syntax error: '%c' col %i", *token, input - token);
            return -1;
        }
    }
    // print remaing token stack
    tokCurs++;
    printStack();
    return 0;
}
```

The `Action`'s functions are quite simple and aren't worth any comments.

## Annexe: How to embed cecho.exe inside a batch file

**Note**: *Debug.exe* has been phased out in Windows 7. I keep this section for reference, but I no longer recommend using it.

The embedding mechanism relies on the *Debug.exe* program. The cecho binary is converted to a debug script using the *BIN2DBG.EXE* utility by Anthony Caruso. The debug script is then embedded in the batch file. At run time, the debug script is processed by *Debug.exe* to recreate the cecho executable file.

1. Convert *cecho.exe* into *cecho.dbg* using *BIN2DBG.EXE*.

```
C:\>bin2dbg.exe cecho.exe
```

2. Edit *cecho.dbg* with a text editor. Modify the file extension on the first line of *cecho.dbg* because *Debug.exe* is not allowed to write an EXE file. Use a *\*.com* extension, for instance.

```
n cecho.com

e 100 4D 5A 90 0 3 0 0 0 4 0 0 0 FF FF 0 0 
e 110 B8 0 0 0 0 0 0 0 40 0 0 0 0 0 0 0 
e 120 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
...
```

3. Include *cecho.dbg* into your batch file. Your batch file must reconstruct exactly the same *cecho.dbg* file. A batch command could be useful:

```
C:\>for /f "delims=" %l in (cecho.dbg) do (echo echo %l ^>^> tmp.dbg >> cecho.bat)
```

4. Add commands to rebuild *cecho.com* (formerly *cecho.exe*) to the end of *cecho.bat*.

```
...
echo e 10E0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0  >> tmp.dbg
echo e 10F0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0  >> tmp.dbg
echo e 1100 0  >> tmp.dbg
echo rcx >> tmp.dbg
echo 1000 >> tmp.dbg
echo w >> tmp.dbg
echo q >> tmp.dbg

echo debug ^< tmp.dbg > tmp.bat
echo exit >> tmp.bat

start /wait /min tmp.bat

del tmp.bat
del tmp.dbg
```

The trick with the *tmp.bat* file and the *start /wait /min* command is detailed by Jakob Bohm in the forum section of this article: [Possible reason for the start /min /wait trick](https://web.archive.org/web/20220309120614/http://www.codeproject.com/Messages/1837431/Possible-reason-for-the-start-min-wait-trick.aspx). It basically runs the 16 bit DOS *Debug.exe* in a separate console to prevent file handles from remaining open.

5. We're done! cecho is now available and ready to colorize your batch output.

```
...
call :unpack_cecho

REM cecho.com is now available for use.
cecho.com {red}This line in red{#}
...
del cecho.com
goto :eof
```

Because the debug script size is much bigger than the EXE size, we need to keep *cecho.exe* as small as possible. The [UPX](https://web.archive.org/web/20220309120614/http://upx.sourceforge.net/) executable packer reduces *cecho.exe* (compiled with VC++10) by 50%.

I also suggest to include the debug script as a "function" in your batch file. See the [batch function syntax by Ritchie Laurence](https://web.archive.org/web/20220309120614/http://www.commandline.co.uk/).

## History

- Initial release - January 03, 2007
- Minor changes - August 07, 2007

- Added the "*start /min /wait*" trick and reduced the EXE size with UPX.

- cecho v2.0 - August 23, 2010

- Added Unicode support.
- Added x64 support.

---

## License note

The original CodeProject publication states that the article and associated source were distributed under the **Code Project Open License (CPOL)**. The v2 file in this repository is explicitly labeled as a reconstruction. The recovered v3 derivative retains its own attribution header and is preserved unchanged for historical reference.