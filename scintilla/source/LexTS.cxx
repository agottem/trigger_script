// Scintilla source code edit control
/** @file LexTS.cxx
 ** Lexer for Trigger Script language.
 **
 ** Written by Paul Winwood.
 ** Folder by Alexey Yutkin.
 ** Modified by Marcos E. Wurzius & Philippe Lhoste
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif


static void ColouriseTSDoc(
    unsigned int startPos,
    int length,
    int initStyle,
    WordList *keywordlists[],
    Accessor &styler) {

    WordList &keywords = *keywordlists[0];
    WordList &keywords2 = *keywordlists[1];
    WordList &keywords3 = *keywordlists[2];
    WordList &keywords4 = *keywordlists[3];
    WordList &keywords5 = *keywordlists[4];
    WordList &keywords6 = *keywordlists[5];
    WordList &keywords7 = *keywordlists[6];
    WordList &keywords8 = *keywordlists[7];

    CharacterSet setWordStart(CharacterSet::setAlpha, "_", 0x80, false);
    CharacterSet setWord(CharacterSet::setAlphaNum, "_", 0x80, false);
    CharacterSet setNumber(CharacterSet::setDigits, ".-+");
    CharacterSet setTSOperator(CharacterSet::setNone, "%^*/-+=()~<>,");
    CharacterSet setEscapeSkip(CharacterSet::setNone, "\"\\");

    int currentLine = styler.GetLine(startPos);

    // Do not leak onto next line
    initStyle = SCE_TS_DEFAULT;

    StyleContext sc(startPos, length, initStyle, styler);
    for (; sc.More(); sc.Forward()) {
        if (sc.atLineEnd) {
            // Update the line state, so it can be seen by next line
            currentLine = styler.GetLine(sc.currentPos);
            styler.SetLineState(currentLine, 0);
        }

        // Determine if the current state should terminate.
        if (sc.state == SCE_TS_OPERATOR) {
            sc.SetState(SCE_TS_DEFAULT);
        } else if (sc.state == SCE_TS_NUMBER) {
            // We stop the number definition on non-numerical non-dot non-eEpP non-sign non-hexdigit char
            if (!setNumber.Contains(sc.ch)) {
                sc.SetState(SCE_TS_DEFAULT);
            } else if (sc.ch == '-' || sc.ch == '+') {
                    sc.SetState(SCE_TS_DEFAULT);
            }
        } else if (sc.state == SCE_TS_IDENTIFIER) {
            if (!setWord.Contains(sc.ch)) {
                char s[100];
                sc.GetCurrent(s, sizeof(s));
                if (keywords.InList(s)) {
                    sc.ChangeState(SCE_TS_WORD1);
                } else if (keywords2.InList(s)) {
                    sc.ChangeState(SCE_TS_WORD2);
                } else if (keywords3.InList(s)) {
                    sc.ChangeState(SCE_TS_WORD3);
                } else if (keywords4.InList(s)) {
                    sc.ChangeState(SCE_TS_WORD4);
                } else if (keywords5.InList(s)) {
                    sc.ChangeState(SCE_TS_WORD5);
                } else if (keywords6.InList(s)) {
                    sc.ChangeState(SCE_TS_WORD6);
                } else if (keywords7.InList(s)) {
                    sc.ChangeState(SCE_TS_WORD7);
                } else if (keywords8.InList(s)) {
                    sc.ChangeState(SCE_TS_WORD8);
                }
                sc.SetState(SCE_TS_DEFAULT);
            }
        } else if (sc.state == SCE_TS_COMMENT) {
            if (sc.atLineEnd) {
                sc.ForwardSetState(SCE_TS_DEFAULT);
            }
        } else if (sc.state == SCE_TS_STRING) {
            if (sc.ch == '\\') {
                if (setEscapeSkip.Contains(sc.chNext)) {
                    sc.Forward();
                }
            } else if (sc.ch == '\"') {
                sc.ForwardSetState(SCE_TS_DEFAULT);
            }
        }

        // Determine if a new state should be entered.
        if (sc.state == SCE_TS_DEFAULT) {
            if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
                sc.SetState(SCE_TS_NUMBER);
            } else if (setWordStart.Contains(sc.ch)) {
                sc.SetState(SCE_TS_IDENTIFIER);
            } else if (sc.ch == '\"') {
                sc.SetState(SCE_TS_STRING);
            } else if (sc.Match('#')) {
                sc.SetState(SCE_TS_COMMENT);
                sc.Forward();
            } else if (setTSOperator.Contains(sc.ch)) {
                sc.SetState(SCE_TS_OPERATOR);
            }
        }
    }

    if (setWord.Contains(sc.chPrev)) {
        char s[100];
        sc.GetCurrent(s, sizeof(s));
        if (keywords.InList(s)) {
            sc.ChangeState(SCE_TS_WORD1);
        } else if (keywords2.InList(s)) {
            sc.ChangeState(SCE_TS_WORD2);
        } else if (keywords3.InList(s)) {
            sc.ChangeState(SCE_TS_WORD3);
        } else if (keywords4.InList(s)) {
            sc.ChangeState(SCE_TS_WORD4);
        } else if (keywords5.InList(s)) {
            sc.ChangeState(SCE_TS_WORD5);
        } else if (keywords6.InList(s)) {
            sc.ChangeState(SCE_TS_WORD6);
        } else if (keywords7.InList(s)) {
            sc.ChangeState(SCE_TS_WORD7);
        } else if (keywords8.InList(s)) {
            sc.ChangeState(SCE_TS_WORD8);
        }
    }

    sc.Complete();
}

static void FoldTSDoc(unsigned int startPos, int length, int /* initStyle */, WordList *[],
                       Accessor &styler) {
    unsigned int lengthDoc = startPos + length;
    int visibleChars = 0;
    int lineCurrent = styler.GetLine(startPos);
    int levelPrev = styler.LevelAt(lineCurrent) & SC_FOLDLEVELNUMBERMASK;
    int levelCurrent = levelPrev;
    char chNext = styler[startPos];
    bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;
    int styleNext = styler.StyleAt(startPos);
    char s[10];

    for (unsigned int i = startPos; i < lengthDoc; i++) {
        char ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);
        int style = styleNext;
        styleNext = styler.StyleAt(i + 1);
        bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
        if (style == SCE_TS_WORD1) {
            if (ch == 'i' || ch == 'l' || ch == 'w' || ch == 'f' || ch == 'a' || ch == 'e') {
                for (unsigned int j = 0; j < 6; j++) {
                    if (!iswordchar(styler[i + j])) {
                        break;
                    }
                    s[j] = styler[i + j];
                    s[j + 1] = '\0';
                }

                if ((strcmp(s, "if") == 0) || (strcmp(s, "loop") == 0) || (strcmp(s, "while") == 0) || (strcmp(s, "for") == 0) || (strcmp(s, "action") == 0)) {
                    levelCurrent++;
                }
                if ((strcmp(s, "end") == 0) || (strcmp(s, "elseif") == 0)) {
                    levelCurrent--;
                }
            }
        } else if (style == SCE_TS_OPERATOR) {
            if (ch == '(') {
                levelCurrent++;
            } else if (ch == ')') {
                levelCurrent--;
            }
        }

        if (atEOL) {
            int lev = levelPrev;
            if (visibleChars == 0 && foldCompact) {
                lev |= SC_FOLDLEVELWHITEFLAG;
            }
            if ((levelCurrent > levelPrev) && (visibleChars > 0)) {
                lev |= SC_FOLDLEVELHEADERFLAG;
            }
            if (lev != styler.LevelAt(lineCurrent)) {
                styler.SetLevel(lineCurrent, lev);
            }
            lineCurrent++;
            levelPrev = levelCurrent;
            visibleChars = 0;
        }
        if (!isspacechar(ch)) {
            visibleChars++;
        }
    }
    // Fill in the real level of the next line, keeping the current flags as they will be filled in later

    int flagsNext = styler.LevelAt(lineCurrent) & ~SC_FOLDLEVELNUMBERMASK;
    styler.SetLevel(lineCurrent, levelPrev | flagsNext);
}

static const char * const tsWordListDesc[] = {
    "Keywords",
    "Plugin Functions",
    "User defined functions",
    "user1",
    "user2",
    "user3",
    "user4",
    "user5",
    0
};

LexerModule lmTS(SCLEX_TS, ColouriseTSDoc, "ts", FoldTSDoc, tsWordListDesc);

