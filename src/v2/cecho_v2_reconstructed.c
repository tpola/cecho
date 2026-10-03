/*
 * cecho.c - reconstructed cecho v2.0
 *
 * Original author: Thomas Polaert
 * Original CodeProject article:
 *   "Add Colors to Batch Files"
 *   http://www.codeproject.com/Articles/17033/Add-Colors-to-Batch-Files
 *
 * cecho v2.0 date from article history: August 23, 2010
 * Article license: Code Project Open License (CPOL)
 *
 * --------------------------------------------------------------------------
 * RECONSTRUCTION NOTICE
 * --------------------------------------------------------------------------
 * The original cecho_src.zip has not been recovered.
 *
 * This file reconstructs the 2010 v2.0 source from:
 *   1. C/C++ source fragments printed in the archived CodeProject article;
 *   2. the documented v2.0 syntax and behavior;
 *   3. a later cecho v3.00 source derived from Thomas Polaert's code.
 *
 * The following parts are directly supported by code printed in the article:
 *   - parser Grammar / Action / Successor tables;
 *   - MAX_TRANS;
 *   - token-stack parser architecture;
 *   - GetCommandLineW based command-line retrieval;
 *   - the wmain parser loop;
 *   - the SetConsoleTextAttribute based setColor() design;
 *   - Unicode syntax {\uXXXX};
 *   - named colors, {{, {\n}, {\t}, {#}.
 *
 * Small action functions not printed in full by the article are reconstructed.
 * They are intentionally kept close to the later v3 source while excluding
 * features that are not documented for v2.0.
 *
 * Not present here because they appear to be later v3 additions:
 *   - -n / -r / -z / -h / -v command-line switches;
 *   - automatic final newline;
 *   - UTF-8 conversion for redirected stdout;
 *   - console/pipe/file output abstraction;
 *   - enhanced invalid-sequence diagnostics;
 *   - "default" named-color alias;
 *   - ntdll.h dependency;
 *   - v3 parser changes.
 */

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

typedef void (*Fct)();

void printStack();
void resetStack();
void setColor();
void printEscChar();
void printHexChar();
void parseColor();

#define MAX_TRANS 5
#define MAX_STACK 0x1000

/*
 * Parser tables reproduced from the v2.0 code block printed in the article.
 */

/* expected tokens list */
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

/* action executed on state changed */
Fct Action[][MAX_TRANS] = {
    {&printStack, 0},
    {&resetStack, 0, 0, 0, 0},
    {&printEscChar, 0},
    {0},
    {0, &printHexChar, &printHexChar, &printHexChar},
    {0, 0},
    {&setColor, 0},
    {&parseColor, 0}
};

/* next state Ids */
char Successor[][MAX_TRANS] = {
    {1, 0},
    {0, 2, 5, 1, 7},
    {1, 3},
    {4},
    {4, 1, 0, 2},
    {6, 7},
    {0, 7},
    {0, 7}
};

/*
 * Available colors documented by the article.
 * The special "#" token restores the initial console colors.
 *
 * "default" is deliberately NOT included: the archived article documents
 * {#}, while a Jan. 2010 reader comment merely requests {default}.
 */
wchar_t* Color[] = {
    L"black",
    L"navy",
    L"green",
    L"teal",
    L"maroon",
    L"purple",
    L"olive",
    L"silver",
    L"gray",
    L"blue",
    L"lime",
    L"aqua",
    L"red",
    L"fuchsia",
    L"yellow",
    L"white",
    L"#"
};

#define COLOR_COUNT (sizeof(Color) / sizeof(Color[0]))

/* token stack */
wchar_t tokStack[MAX_STACK];

/* token stack cursor */
int tokCurs = 0;

/* initial console attributes, used by {#} */
WORD defaultTxtAttr = 7;


/*
 * Print the text accumulated before a formatting sequence.
 *
 * The state-0 transition pushes '{' before calling this function, so the
 * last character is replaced by NUL. At end-of-input wmain increments
 * tokCurs first, which provides the same terminating slot.
 */
void printStack()
{
    tokStack[tokCurs - 1] = 0;
    wprintf(L"%s", tokStack);
}


/*
 * Handle the "{{" escape sequence.
 *
 * State 1 also sends "{}" here. Only "{{" is documented as an escape;
 * the closing brace case therefore emits nothing.
 */
void resetStack()
{
    if (tokStack[tokCurs - 1] == L'{')
        wprintf(L"{");
}


/*
 * Apply a two-digit hexadecimal console color.
 *
 * The archived article prints this function with strtol(). Since the v2
 * parser and stack are wchar_t-based, wcstol() is used here so the
 * reconstructed source is buildable as Unicode C.
 */
void setColor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    tokStack[tokCurs - 1] = 0;

    SetConsoleTextAttribute(
        hConsole,
        (WORD)wcstol(tokStack, 0, 16));
}


/*
 * Print documented escape characters.
 */
void printEscChar()
{
    switch (tokStack[tokCurs - 1])
    {
        case L't':
            wprintf(L"\t");
            break;

        case L'n':
            wprintf(L"\n");
            break;
    }
}


/*
 * Print a Unicode character represented by {\uXXXX}.
 *
 * Depending on the state transition, tokStack contains either:
 *   "\\uXXXX<delimiter>"
 * or a continuation introduced by another backslash inside the same braces.
 */
void printHexChar()
{
    wchar_t* hex;
    wchar_t saved;
    wchar_t chr;

    /*
     * Grammar/action behavior resets tokStack after every emitted Unicode
     * character. The documented forms therefore reach us beginning with
     * "\\u" for a fresh escape.
     */
    if (tokStack[0] == L'\\' && tokStack[1] == L'u')
        hex = tokStack + 2;
    else
        hex = tokStack;

    /* The transition character (space, '}', or '\') is the last token. */
    saved = tokStack[tokCurs - 1];
    tokStack[tokCurs - 1] = 0;

    chr = (wchar_t)wcstol(hex, 0, 16);
    wprintf(L"%lc", chr);

    tokStack[tokCurs - 1] = saved;
}


/*
 * Parse a human-readable color specification.
 *
 * Documented examples:
 *   {red}
 *   {black on blue}
 *   {light red on black}
 *   {#}
 *
 * Non-color words such as "on" and "light" are ignored. The first
 * recognized color is the foreground, the second is the background.
 * If only a foreground is specified, the original background is kept.
 */
void parseColor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD txtAttr = 0;
    int shift = 0;
    wchar_t* pos;
    wchar_t* end;

    tokStack[tokCurs - 1] = 0;

    pos = tokStack;
    end = tokStack + wcslen(tokStack);

    while (pos < end)
    {
        wchar_t* token;
        wchar_t* p;
        size_t clr;

        while (pos < end && iswspace(*pos))
            pos++;

        if (pos >= end)
            break;

        token = pos;

        while (pos < end && !iswspace(*pos))
            pos++;

        if (pos < end)
        {
            *pos = 0;
            pos++;
        }

        /*
         * Ignore the English connective used by the documented syntax.
         * Other unrecognized words (e.g. "light") are ignored too.
         */
        for (clr = 0; clr < COLOR_COUNT; clr++)
        {
            if (_wcsicmp(token, Color[clr]) == 0)
            {
                if (clr == 16) /* "#" */
                {
                    txtAttr |= (defaultTxtAttr & (0x0F << shift));
                }
                else
                {
                    txtAttr |= ((WORD)clr << shift);
                }

                shift += 4;
                break;
            }
        }

        if (shift >= 8)
            break;

        /* Silence old compiler warnings about p in minimal VS projects. */
        p = 0;
        (void)p;
    }

    if (shift == 0)
        return;

    /* Only foreground supplied: preserve initial background. */
    if (shift == 4)
        txtAttr |= (defaultTxtAttr & 0xF0);

    SetConsoleTextAttribute(hConsole, txtAttr);
}


/*
 * Main parser loop reconstructed from the v2.0 article.
 */
int wmain(int argc, wchar_t* argv[])
{
    short trans, clr, fired;
    wchar_t* token;
    char state = 0;
    LPWSTR input;
    HANDLE hConsole;
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

    (void)argc;
    (void)clr;

    /* Save initial console colors for {#}. */
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE &&
        GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
    {
        defaultTxtAttr = consoleInfo.wAttributes;
    }

    /*
     * Retrieve command argument.
     *
     * This is the GetCommandLineW approach added to the article in Aug. 2010
     * after discussion about preserving repeated spaces.
     */
    input = GetCommandLineW();

    if (*input == L'"')
        input += 2;

    input += wcslen(argv[0]) + 1;

    /* parse input string */
    for (fired = 0, token = input; *token != 0; token++)
    {
        /* does token trigger a transition ? */
        for (trans = 0;
             trans < MAX_TRANS && Grammar[state][trans] != 0;
             trans++)
        {
            /*
             * check if token belongs to the expected tokens list ?
             * an empty list acts as a wildcard token
             */
            if (wcschr(Grammar[state][trans], *token) ||
                *Grammar[state][trans] == 0)
            {
                /* push token into the stack */
                if (tokCurs < MAX_STACK)
                {
                    tokStack[tokCurs++] = *token;
                }
                else
                {
                    wprintf(L"Error: Max stack size reached (%d)\n",
                            MAX_STACK);
                    return -1;
                }

                /* execute the action associated to the transition (if any) */
                if (Action[state][trans] != 0)
                {
                    (*Action[state][trans])();
                    tokCurs = 0; /* reset token stack */
                }

                /* update parser state */
                state = Successor[state][trans];
                fired = 1;
                break;
            }
        }

        if (fired == 0)
        {
            /*
             * The article prints input - token here, which would produce
             * negative positions. token - input is used in this buildable
             * reconstruction.
             */
            wprintf(L"Syntax error: '%c' col %i",
                    *token,
                    (int)(token - input));

            return -1;
        }

        fired = 0;
    }

    /* print remaining token stack */
    tokCurs++;
    printStack();

    return 0;
}