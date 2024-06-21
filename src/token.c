#include "token.h"

TokenMap hmTokens;

void
initTokenHashMap()
{
    hmTokens = TokenMapCreate(64); /* no collisions with 64 */

    TokenMapInsert(&hmTokens, (StrToken){.str = "const", .token = TOK_CONST});
    TokenMapInsert(&hmTokens, (StrToken){.str = "var", .token = TOK_VAR});
    TokenMapInsert(&hmTokens, (StrToken){.str = "procedure", .token = TOK_PROCEDURE});
    TokenMapInsert(&hmTokens, (StrToken){.str = "call", .token = TOK_CALL});
    TokenMapInsert(&hmTokens, (StrToken){.str = "begin", .token = TOK_BEGIN});
    TokenMapInsert(&hmTokens, (StrToken){.str = "end", .token = TOK_END});
    TokenMapInsert(&hmTokens, (StrToken){.str = "if", .token = TOK_IF});
    TokenMapInsert(&hmTokens, (StrToken){.str = "then", .token = TOK_THEN});
    TokenMapInsert(&hmTokens, (StrToken){.str = "while", .token = TOK_WHILE});
    TokenMapInsert(&hmTokens, (StrToken){.str = "do", .token = TOK_DO});
    TokenMapInsert(&hmTokens, (StrToken){.str = "odd", .token = TOK_ODD});
    TokenMapInsert(&hmTokens, (StrToken){.str = "writeInt", .token = TOK_WRITEINT});
    TokenMapInsert(&hmTokens, (StrToken){.str = "writeChar", .token = TOK_WRITECHAR});
    TokenMapInsert(&hmTokens, (StrToken){.str = "readInt", .token = TOK_READINT});
    TokenMapInsert(&hmTokens, (StrToken){.str = "readChar", .token = TOK_READCHAR});
    TokenMapInsert(&hmTokens, (StrToken){.str = "into", .token = TOK_INTO});
}

void
destroyTokenHashMap()
{
    TokenMapClean(&hmTokens);
}
