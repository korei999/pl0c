#include "logs.h"
#include "token.h"
#include "adt/list.h"
#include "adt/array.h"
#include "strtonum.h"

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

#define CHECK_LHS	0
#define CHECK_RHS	1
#define CHECK_CALL	2

typedef struct SymNode
{
    int depth;
    int type;
    long size;
    char* name;
} SymNode;

static inline int
SymNodeCmp(const SymNode n0, const SymNode n1)
{
    return strcmp(n0.name, n1.name);
}

static inline size_t
SymNodeHash(const SymNode n0)
{
    return hashFNV(n0.name);
}

HASHMAP_GEN_CODE(SymMap, SymNode, SymNodeHash, SymNodeCmp, ADT_HASHMAP_DEFAULT_LOAD_FACTOR);
LIST_GEN_CODE(SymList, SymNode, SymNodeCmp);
typedef char* pChar;
ARRAY_GEN_CODE(ArrStr, pChar);

static void expression(void);

char* raw, * token;
static int type;
static size_t line = 1;
static int depth = 0;
static int proc = 0;

SymMap symmap;
SymList symtab;
ArrStr aClean;
SymList lClean;

static void
error(const char* fmt, ...)
{
    va_list ap;

    CERR("pl0c: error: %lu: ", line);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    CERR("\n");
    exit(1);
}

void
printToken(void)
{
    COUT("(%lu|%s): ", line, tokenStrings[type]);
    switch (type)
    {
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

static void
initSymtab(void)
{
    symmap = SymMapCreate(ADT_DEFAULT_SIZE);
    symtab = SymListCreate();
    lClean = SymListCreate();
    aClean = ArrStrCreate(ADT_DEFAULT_SIZE);
    SymListPushBack(&symtab, (SymNode){.depth = 0, .name = "main", .type = TOK_PROCEDURE});
    /*SymMapInsert(&symmap, (SymNode){.depth = 0, .name = "main", .type = TOK_PROCEDURE});*/
}

static void
destroySymbols(void)
{
    /*COUT("killing: ...\n");*/
    LIST_FOREACH_REV_SAFE(&symtab, it, itmp)
    {
        if (it->data.type != TOK_PROCEDURE)
        {
            /*free(it->data.name);*/
            /*COUT("\t'%s'\n", it->data.name);*/
            SymListRemove(&symtab, it);
        }
        else
        {
            break;
        }
    }
}

static void
destroySymtab(void)
{
    // for (size_t i = 0; i < aClean.size; i++)
        // free(aClean.pData[i]);

    // ArrStrClean(&aClean);
    SymListClean(&symtab);
    SymMapClean(&symmap);
}

static void
addSymbol(int type)
{
    /*CERR("adding: '%s'\n", token);*/

    SymListNode* curr;

    curr = symtab.pFirst;

    while (true)
    {
        if (!strcmp(curr->data.name, token))
            if (curr->data.depth == (depth - 1))
                error("duplicate symbol: %s", token);

        if (!curr->pNext)
            break;

        curr = curr->pNext;
    }

    /*SymMapReturnNode f = SymMapTryInsert(&symmap);*/

    char* n = strdup(token);
    /*CERR("added: '%s'\n", n);*/
    ArrStrPush(&aClean, n);
    SymListPushBack(&symtab, (SymNode){.depth = depth - 1, .type = type, .name = n});
    /*SymMapInsert(&symmap, (SymNode){.depth = depth - 1, .type = type, .name = n});*/
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

    TokenMapReturnNode f = TokenMapSearchValue(&hmTokens, token);
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

/* Code generator */

static void
cgEnd(void)
{
    COUT("\n/* PL/0 compiler %g */\n", PL0C_VERSION);
}

static void
cgConst(void)
{
    COUT("const long %s = ", token);
}

static void
cgSemicolon(void)
{
    COUT(";\n");
}

static void
cgSymbol(void)
{
    switch (type)
    {
        case TOK_IDENT:
        case TOK_NUMBER:
            COUT("%s", token);
            break;

        case TOK_BEGIN:
            COUT("{\n");
            break;

        case TOK_END:
            COUT(";\n}\n");
            break;

        case TOK_IF:
            COUT("if(");
            break;

        case TOK_THEN:
        case TOK_DO:
            COUT(")");
            break;

        case TOK_ODD:
            COUT("(");
            break;

        case TOK_WHILE:
            COUT("while (");
            break;

        case TOK_EQUAL:
            COUT("==");
            break;

        case TOK_COMMA:
            COUT(",");
            break;

        case TOK_ASSIGN:
            COUT("=");
            break;

        case TOK_HASH:
            COUT("!=");
            break;

        case TOK_LESSTHAN:
            COUT("<");
            break;

        case TOK_GREATERTHAN:
            COUT(">");
            break;

        case TOK_PLUS:
            COUT("+");
            break;

        case TOK_MINUS:
            COUT("-");
            break;

        case TOK_MULTIPLY:
            COUT("*");
            break;

        case TOK_DIVIDE:
            COUT("/");
            break;

        case TOK_LPAREN:
            COUT("(");
            break;

        case TOK_RPAREN:
            COUT(")");
            break;
    }
}

static void
cgCrlf(void)
{
    COUT("\n");
}

static void
cgVar(void)
{
    COUT("long %s;\n", token);
}

static void
cgProcedure(void)
{
    if (proc == 0)
    {
        COUT("int\n");
        COUT("main(int argc, char* argv[])\n");
    }
    else
    {
        COUT("void\n");
        COUT("%s(void)\n", token);
    }

    COUT("{\n");
}

static void
cgEpilogue(void)
{
    COUT(";");
    if (proc == 0)
        COUT("return 0;");
    COUT("\n}\n\n");
}

static void
cgCall(void)
{
    COUT("%s();\n", token);
}

static void
cgOdd(void)
{
    COUT(")&1");
}

static void
cgWriteChar(void)
{
    COUT("(void)fprintf(stdout, \"%%c\", (unsigned char) %s);", token);
}

static void
cgWriteInt(void)
{
    COUT("(void)fprintf(stdout, \"%%ld\", (long) %s);", token);
}

static void
cgInit(void)
{
    COUT("#include <stdio.h>\n");
    COUT("#include \"include/strtonum.h\"\n\n");
    COUT("static char __stdin[24];\n");
    COUT("static const char *__errstr;\n");
    COUT("static long __writestridx;\n\n");
}

static void
cgReadChar(void)
{
    COUT("%s=(unsigned char)fgetc(stdin);", token);
}

static void
cgReadInt(void)
{
    COUT("(void)fgets(__stdin, ssizeof(__stdin), stdin);\n");
    COUT("if(__stdin[strlen(__stdin) - 1] == '\\n')");
    COUT("__stdin[stdlen(__stdin) - 1] = '\\0';");
    COUT("%s=(long)strtonum(__stdin, LONG_MIN, LONG_MAX, &__errstr);\n", token);
    COUT("if(__errstr!=NULL){");
    COUT("(void)fprintf(stderr, \"invalid number: %%s\\n\", __stdin);");
    COUT("exit(1);");
    COUT("}");
}

static void
cgArray(void)
{
    COUT("[%s]", token);
}

static void
cgWriteStr(void)
{
    SymListNode* ret = nullptr;

    if (type == TOK_IDENT)
    {
        LIST_FOREACH(&symtab, it)
            if (!strcmp(it->data.name, token))
                ret = it;

        if (!ret)
            error("undefined symbol: '%s'", token);

        if (ret->data.size == 0)
            error("writeStr requires an array");

        COUT("__writestridx = 0;\n");
        COUT("while(%s[__writestridx]!='\\0'&&__writestridx<%ld)\n", token, ret->data.size);
        COUT("(void)fputc((unsigned char)%s[__writestridx++],stdout);\n", token);
    }
    else
    {
        COUT("(void)fprintf(stdout, %s);\n", token);
    }
}

/* Semantics */

static void
symCheck(int check)
{
    SymListNode* ret = nullptr;
    
    LIST_FOREACH(&symtab, it)
        if (!strcmp(token, it->data.name))
            ret = it;

    if (ret == nullptr)
        error("undefined symbol :%s", token);

    switch (check)
    {
        case CHECK_LHS:
            if (ret->data.type != TOK_VAR)
                error("must be a variable: %s", token);
            break;

        case CHECK_RHS:
            if (ret->data.type == TOK_PROCEDURE)
                error("must not be a procedure: %s", token);
            break;

        case CHECK_CALL:
            if (ret->data.type != TOK_PROCEDURE)
                error("must be a procedure: %s", token);
            break;
    }
}

static void
arraySize(void)
{
    const char* errstr;

    if (symtab.pLast->data.type != TOK_VAR)
        error("arrays must be declared with \"var\"");

    symtab.pLast->data.size = strtonum(token, 1, LONG_MAX, &errstr);
    if (errstr)
        error("invalid array size");
}

static void
arrayCheck(void)
{
    SymListNode* ret = nullptr;

    LIST_FOREACH(&symtab, it)
        if (!strcmp(token, it->data.name))
            ret = it;

    if (!ret)
        error("undefined symbol: '%s'", token);
}

/* Parser */

static void
next(void)
{
    type = lex();
    ++raw;

    /*COUT("list: ...\n");*/
    /*LIST_FOREACH(&symtab, it)*/
    /*    COUT("(%s|%s), ", tokenStrings[it->data.type], it->data.name);*/
    /*COUT("\n");*/

    /*printToken();*/
}

static void
expect(int match)
{
    if (match != type)
        error("syntax error: expected: %s, got %s\n", tokenStrings[match], tokenStrings[type]);
    next();
}

static void
factor(void)
{
    switch (type)
    {
        case TOK_IDENT:
            symCheck(CHECK_RHS);
            cgSymbol();
            expect(TOK_IDENT);
            if (type == TOK_LBRACK)
            {
                arrayCheck();
                cgSymbol();
                expect(TOK_LBRACK);
                expression();
                if (type == TOK_LBRACK)
                    cgSymbol();
                expect(TOK_RBRACK);
            }
            break;

        case TOK_NUMBER:
            cgSymbol();
            next();
            break;

        case TOK_LPAREN:
            cgSymbol();
            expect(TOK_LPAREN);
            expression();
            if (type == TOK_RPAREN)
                cgSymbol();
            expect(TOK_RPAREN);
            break;
    }
}

static void
term(void)
{
    factor();

    while (type == TOK_MULTIPLY || type == TOK_DIVIDE)
    {
        cgSymbol();
        next();
        factor();
    }
}

static void
expression(void)
{
    if (type == TOK_PLUS || type == TOK_MINUS)
    {
        cgSymbol();
        next();
    }

    term();

    while (type == TOK_PLUS || type == TOK_MINUS)
    {
        cgSymbol();
        next();
        term();
    }
}

static void
condition(void)
{
    if (type == TOK_ODD)
    {
        cgSymbol();
        expect(TOK_ODD);
        expression();
        cgOdd();
    }
    else
    {
        expression();

        switch (type)
        {
            case TOK_EQUAL:
            case TOK_HASH:
            case TOK_LESSTHAN:
            case TOK_GREATERTHAN:
                cgSymbol();
                next();
                break;

            default:
                error("invalid conditional");
        }

        expression();
    }
}

static void
statement(void)
{
    switch (type)
    {
        case TOK_IDENT:
            symCheck(CHECK_LHS);
            cgSymbol();
            expect(TOK_IDENT);
            if (type == TOK_LBRACK)
            {
                arrayCheck();
                cgSymbol();
                expect(TOK_LBRACK);
                expression();
                if (type == TOK_RBRACK)
                    cgSymbol();
                expect(TOK_RBRACK);
            }
            if (type == TOK_ASSIGN)
                cgSymbol();
            expect(TOK_ASSIGN);
            expression();
            break;

        case TOK_CALL:
            expect(TOK_CALL);
            if (type == TOK_IDENT)
            {
                symCheck(CHECK_CALL);
                cgCall();
            }
            expect(TOK_IDENT);
            break;

        case TOK_BEGIN:
            cgSymbol();
            expect(TOK_BEGIN);
            statement();
            while (type == TOK_SEMICOLON)
            {
                cgSemicolon();
                expect(TOK_SEMICOLON);
                statement();
            }
            if (type == TOK_END)
                cgSymbol();
            expect(TOK_END);
            break;

        case TOK_IF:
            cgSymbol();
            expect(TOK_IF);
            condition();
            if (type == TOK_THEN)
                cgSymbol();
            expect(TOK_THEN);
            statement();
            break;

        case TOK_WHILE:
            cgSymbol();
            expect(TOK_WHILE);
            condition();
            if (type == TOK_DO)
                cgSymbol();
            expect(TOK_DO);
            statement();
            break;

        case TOK_WRITEINT:
            expect(TOK_WRITEINT);
            if (type == TOK_IDENT || type == TOK_NUMBER)
            {
                if (type == TOK_IDENT)
                    symCheck(CHECK_RHS);
                cgWriteInt();
            }

            if (type == TOK_IDENT)
                expect(TOK_IDENT);
            else if (type == TOK_NUMBER)
                expect(TOK_NUMBER);
            else
                error("writeInt takes an identifier or a number");
            break;

        case TOK_WRITECHAR:
            expect(TOK_WRITECHAR);
            if (type == TOK_IDENT || type == TOK_NUMBER)
            {
                if (type == TOK_IDENT)
                    symCheck(CHECK_RHS);
                cgWriteChar();
            }

            if (type == TOK_IDENT)
                expect(TOK_IDENT);
            else if (type == TOK_NUMBER)
                expect(TOK_NUMBER);
            else
                error("writeChar takes an identifier or a number");
            break;

        case TOK_READINT:
            expect(TOK_READINT);
            if (type == TOK_INTO)
                expect(TOK_INTO);

            if (type == TOK_IDENT)
            {
                symCheck(CHECK_LHS);
                cgReadInt();
            }

            expect(TOK_IDENT);
            break;

        case TOK_READCHAR:
            expect(TOK_READCHAR);
            if (type == TOK_INTO)
                expect(TOK_INTO);

            if (type == TOK_IDENT)
            {
                symCheck(CHECK_LHS);
                cgReadChar();
            }
            expect(TOK_IDENT);
            break;

        case TOK_WRITESTR:
            expect(TOK_WRITESTR);
            if (type == TOK_IDENT || type == TOK_STRING)
            {
                if (type == TOK_IDENT)
                    symCheck(CHECK_LHS);
                cgWriteStr();

                if (type == TOK_IDENT)
                    expect(TOK_IDENT);
                else
                    expect(TOK_STRING);
            }
            else
            {
                error("writeStr takes an array or a string");
            }
            break;
    }
}

static void
block(void)
{
    if (depth++ > 1)
        error("nesting depth exceeded");

    if (type == TOK_CONST)
    {
        expect(TOK_CONST);
        if (type == TOK_IDENT)
        {
            addSymbol(TOK_CONST);
            cgConst();
        }
        expect(TOK_IDENT);
        expect(TOK_EQUAL);
        if (type == TOK_NUMBER)
        {
            cgSymbol();
            cgSemicolon();
        }
        expect(TOK_NUMBER);
        while (type == TOK_COMMA)
        {
            expect(TOK_COMMA);
            if (type == TOK_IDENT)
            {
                addSymbol(TOK_CONST);
                cgConst();
            }
            expect(TOK_IDENT);
            expect(TOK_EQUAL);
            if (type == TOK_NUMBER)
            {
                cgSymbol();
                cgSemicolon();
            }
            expect(TOK_NUMBER);
        }
        expect(TOK_SEMICOLON);
    }

    if (type == TOK_VAR)
    {
        expect(TOK_VAR);
        if (type == TOK_IDENT)
        {
            addSymbol(TOK_VAR);
            cgVar();
        }
        expect(TOK_IDENT);
        if (type == TOK_SIZE)
        {
            expect(TOK_SIZE);
            if (type == TOK_NUMBER)
            {
                arraySize();
                cgArray();
            }
            expect(TOK_NUMBER);
        }
        cgSemicolon();
        while (type == TOK_COMMA)
        {
            expect(TOK_COMMA);
            if (type == TOK_IDENT)
            {
                addSymbol(TOK_VAR);
                cgVar();
            }
            expect(TOK_IDENT);
            if (type == TOK_SIZE)
            {
                arraySize();
                cgArray();
                expect(TOK_NUMBER);
            }
            cgSemicolon();
        }
        expect(TOK_SEMICOLON);
        cgCrlf();
    }

    while (type == TOK_PROCEDURE)
    {
        proc = 1;

        expect(TOK_PROCEDURE);
        if (type == TOK_IDENT)
        {
            addSymbol(TOK_PROCEDURE);
            cgProcedure();
        }
        expect(TOK_IDENT);
        expect(TOK_SEMICOLON);

        block();

        expect(TOK_SEMICOLON);

        proc = 0;

        destroySymbols();
    }

    if (proc == 0)
        cgProcedure();

    statement();

    cgEpilogue();

    if (--depth < 0)
        LOG_FATAL("nesting depth fell below 0");
}

static void
parse(void)
{
    cgInit();

    next();
    block();
    expect(TOK_DOT);

    if (type != 0)
        error("extra tokens at end of file");

    cgEnd();
}

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
    initSymtab();

    parse();

    /*LIST_FOREACH(&symtab, it)*/
    /*    COUT("%s\n", it->data.name);*/

    free(startp);
    destroySymtab();
    destroyTokenHashMap();

    return 0;
}
