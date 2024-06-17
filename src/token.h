#pragma once
#include "adt/hashmap.h"
#include "misc.h"
#include <string.h>

typedef struct StrToken
{
    char* str;
    char token;
} StrToken;

static inline int
TokenCmp(StrToken s1, StrToken s2)
{
    return strcmp(s1.str, s2.str);
}

static inline size_t
TokenHash(StrToken s)
{
    return hashMurmurOAAT64(s.str);
}

HASHMAP_GEN_CODE(TokenMap, StrToken, TokenHash, TokenCmp, ADT_HASHMAP_DEFAULT_LOAD_FACTOR);

#define TOK_IDENT 'I'
#define TOK_NUMBER 'N'
#define TOK_CONST 'C'
#define TOK_VAR 'V'
#define TOK_PROCEDURE 'P'
#define TOK_CALL 'c'
#define TOK_BEGIN 'B'
#define TOK_END 'E'
#define TOK_IF 'i'
#define TOK_THEN 'T'
#define TOK_WHILE 'W'
#define TOK_DO 'D'
#define TOK_ODD 'O'
#define TOK_DOT '.'
#define TOK_EQUAL '='
#define TOK_COMMA ','
#define TOK_SEMICOLON ';'
#define TOK_ASSIGN ':'
#define TOK_HASH '#'
#define TOK_LESSTHAN '<'
#define TOK_GREATERTHAN '>'
#define TOK_PLUS '+'
#define TOK_MINUS '-'
#define TOK_MULTIPLY '*'
#define TOK_DIVIDE '/'
#define TOK_LPAREN '('
#define TOK_RPAREN ')'

extern TokenMap hmTokens;

static const char* tokenStrings[] = {
    [TOK_IDENT] = "IDENT",
    [TOK_NUMBER] = "NUMBER",
    [TOK_CONST] = "CONST",
    [TOK_VAR] = "VAR",
    [TOK_PROCEDURE] = "PROCEDURE",
    [TOK_CALL] = "CALL",
    [TOK_BEGIN] = "BEGIN",
    [TOK_END] = "END",
    [TOK_IF] = "IF",
    [TOK_THEN] = "THEN",
    [TOK_WHILE] = "WHILE",
    [TOK_DO] = "DO",
    [TOK_ODD] = "ODD",
    [TOK_DOT] = "DOT",
    [TOK_EQUAL] = "EQUAL",
    [TOK_COMMA] = "COMMA",
    [TOK_SEMICOLON] = "SEMICOLON",
    [TOK_ASSIGN] = "ASSIGN",
    [TOK_HASH] = "HASH",
    [TOK_LESSTHAN] = "LESSTHAN",
    [TOK_GREATERTHAN] = "GREATERTHAN",
    [TOK_PLUS] = "PLUS",
    [TOK_MINUS] = "MINUS",
    [TOK_MULTIPLY] = "MULTIPLY",
    [TOK_DIVIDE] = "DIVIDE",
    [TOK_LPAREN] = "LPAREN",
    [TOK_RPAREN] = "RPAREN"
};

void initTokenHashMap();
void destroyTokenHashMap();
