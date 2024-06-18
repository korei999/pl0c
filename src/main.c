#include "logs.h"
#include "token.h"

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __linux__
    #include <bsd/stdlib.h>
#else
    #include <stdlib.h>
#endif

/*
 * pl0c -- PL/0 compiler.
 *
 * program	    = block "." .
 * block	    = [ "const" ident "=" number { "," ident "=" number } ";" ]
 *		          [ "var" ident { "," ident } ";" ]
 *		          { "procedure" ident ";" block ";" } statement .
 * statement	= [ ident ":=" expression
 *		          | "call" ident
 *		          | "begin" statement { ";" statement } "end"
 *		          | "if" condition "then" statement
 *		          | "while" condition "do" statement ] .
 * condition	= "odd" expression
 *		          | expression ( "=" | "#" | "<" | ">" ) expression .
 * expression	= [ "+" | "-" ] term { ( "+" | "-" ) term } .
 * term		    = factor { ( "*" | "/" ) factor } .
 * factor	    = ident
 *		        | number
 *		        | "(" expression ")" .
 */

char* raw, * token;
static int type;
static size_t line = 1;
static int depth = 0;

static void
error(const char* fmt, ...)
{
    va_list ap;

    CERR("pl0c: error: %lu: ", line);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_start(ap, fmt);

    CERR("\n");
    exit(1);
}

static void
readin(char* file)
{
    int fd;
    struct stat st;

    if (strrchr(file, '.') == nullptr)
        error("file must end in '.pl0'");

    if(strcmp(strrchr(file, '.'), ".pl0") != 0)
        error("file must end in '.pl0'");

    if ((fd = open(file, O_RDONLY)) == -1)
        error("couldn't open %s", file);

    if (fstat(fd, &st) == -1)
        error("couldn't get file size");

    if ((raw = malloc(st.st_size + 1)) == nullptr)
        LOG_FATAL("malloc failed");

    if (read(fd, raw, st.st_size) != st.st_size)
        error("couldn't read %s", file);

    raw[st.st_size] = '\0';
    close(fd);
}

/* Lexer */

static void
comment(void)
{
    int ch;
    
    while ((ch = *raw++) != '}')
    {
        if (ch == '\0')
            error("unterminated comment");
        if (ch == '\n')
            ++line;
    }
}

static int
ident(void)
{
    char* p;
    size_t i, len;

    p = raw;
    while (isalnum(*raw) || *raw == '_')
        ++raw;

    len = raw - p;

    --raw;

    free(token);

    if ((token = malloc(len + 1)) == nullptr)
        LOG_FATAL("malloc failed");

    for (i = 0; i < len; i++)
        token[i] = *p++;
    token[i] = '\0';

    auto f = TokenMapSearch(&hmTokens, (StrToken){.str = token});
    if (f.pData)
        return f.pData->token;

    return TOK_IDENT;
}

static int
number(void)
{
    const char* errstr;
    char* p;
    size_t i, j = 0, len;

    p = raw;
    while (isdigit(*raw) || *raw == '_')
        ++raw;

    len = raw - p;

    --raw;

    free(token);

    if ((token = malloc(len + 1)) == nullptr)
        LOG_FATAL("malloc\n");

    for (i = 0; i < len; i++)
    {
        if (isdigit(*p))
            token[j++] = *p;
        ++p;
    }
    token[j] = '\0';

    strtonum(token, 0, LONG_MAX, &errstr);
    if (errstr)
        LOG_FATAL("invalid number: %s\n", token);

    return TOK_NUMBER;
}

static int
lex(void)
{
again:
    /* skip whitespace */
    while (*raw == ' ' || *raw == '\t' || *raw =='\n')
        if (*raw++ == '\n')
            ++line;

    if (isalpha(*raw) || *raw == '_')
        return ident();

    if (isdigit(*raw))
        return number();

    switch (*raw)
    {
        case '{':
            comment();
            goto again;
        case '.':
        case '=':
        case ',':
        case ';':
        case '#':
        case '<':
        case '>':
        case '+':
        case '-':
        case '*':
        case '/':
        case '(':
        case ')':
            return (*raw);
        case ':':
            if (*++raw != '=')
                LOG_BAD("unknown token: ':%c'\n", *raw);

            return TOK_ASSIGN;
        case '\0':
            return 0;
        default:
            LOG_BAD("unknown token: '%c'\n", *raw);
    }

    return 0;
}

/* Parser */

static void
next(void)
{
    type = lex();
    ++raw;
}

static void
expect(int match)
{
    if (match != type)
        error("syntax error");
    next();
}

static void
parse(void)
{
    while ((type = lex()) != 0)
    {
        ++raw;
        COUT("(%lu|%s) ", line, tokenStrings[type]);
        switch (type) {
            case TOK_IDENT:
            case TOK_NUMBER:
            case TOK_CONST:
            case TOK_VAR:
            case TOK_PROCEDURE:
            case TOK_CALL:
            case TOK_BEGIN:
            case TOK_END:
            case TOK_IF:
            case TOK_THEN:
            case TOK_WHILE:
            case TOK_DO:
            case TOK_ODD:
                COUT("'%s'", token);
                break;
            case TOK_DOT:
            case TOK_EQUAL:
            case TOK_COMMA:
            case TOK_SEMICOLON:
            case TOK_HASH:
            case TOK_LESSTHAN:
            case TOK_GREATERTHAN:
            case TOK_PLUS:
            case TOK_MINUS:
            case TOK_MULTIPLY:
            case TOK_DIVIDE:
            case TOK_LPAREN:
            case TOK_RPAREN:
                fputc(type, stdout);
                break;
            case TOK_ASSIGN:
                fputs(":=", stdout);
        }
        fputc('\n', stdout);
    }
}

static void
block(void)
{
    if (depth ++ > 1)
        error("nesting depth exceeded");

    if (type == TOK_CONST)
    {
        expect(TOK_CONST);
        expect(TOK_IDENT);
        expect(TOK_EQUAL);
        expect(TOK_NUMBER);

        while (type == TOK_COMMA)
        {
            expect(TOK_COMMA);
            expect(TOK_IDENT);
            expect(TOK_EQUAL);
            expect(TOK_NUMBER);
        }
        expect(TOK_SEMICOLON);
    }

    if (type == TOK_VAR)
    {
        expect(TOK_VAR);
        expect(TOK_IDENT);

        while (type == TOK_COMMA)
        {
            expect(TOK_COMMA);
            expect(TOK_IDENT);
        }
        expect(TOK_SEMICOLON);
    }

    while (type == TOK_PROCEDURE)
    {
        expect(TOK_PROCEDURE);
        expect(TOK_IDENT);
        expect(TOK_SEMICOLON);

        block();

        expect(TOK_SEMICOLON);
    }

    /*statement();*/

    if (--depth < 0)
        LOG_FATAL("nesting depth fell below 0");
}

/*static void*/
/*_parse(void)*/
/*{*/
/*    next();*/
/*    block();*/
/*    expect(TOK_DOT);*/
/**/
/*    if (type != 0)*/
/*        error("extra tokens at end of file");*/
/*}*/

int
main(int argc, char* argv[])
{
    char* startp;

    if (argc != 2)
    {
        CERR("usage: pl0c file.pl0\n");
        exit(1);
    }

    readin(argv[1]);
    startp = raw;

    initTokenHashMap();

    parse();

    free(startp);
    destroyTokenHashMap();

    return 0;
}
