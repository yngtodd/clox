#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "chunk.h"
#include "value.h"
#include "scanner.h"

typedef struct
{
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

Parser parser;
Chunk* chunk_compiling;

static Chunk* chunk_current()
{
    return chunk_compiling;
}

static void error_at(Token* token, const char* message)
{
    if (parser.panic_mode) return;
    parser.panic_mode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // Nothing
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void error_at_current(const char* message)
{
    error_at(&parser.previous, message);
}

static void error(const char* message)
{
    error_at(&parser.previous, message);
}


static void advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = token_scan();
        if (parser.current.type != TOKEN_ERROR) break;

        error_at_current(parser.current.start);
    }
}

static void consume(TokenType type, const char* message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }

    error_at_current(message);
}

static void byte_emit(uint8_t byte)
{
    chunk_write(chunk_current(), byte, parser.previous.line);
}

static void bytes_emit(uint8_t byte1, uint8_t byte2)
{
    byte_emit(byte1);
    byte_emit(byte2);
}

static void return_emit()
{
    byte_emit(OP_RETURN);
}

static void compiler_end()
{
    return_emit();
}

static uint8_t constant_make(Value value)
{
    // Append constant to the chunk's constant table, and get it's index
    int constant = chunk_constant_add(chunk_current(), value);

    if (constant > UINT8_MAX)
    {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void constant_emit(Value value)
{
    bytes_emit(OP_CONSTANT, constant_make(value));
}

static void number()
{
    double value = strtod(parser.previous.start, NULL);
    constant_emit(value);
}

static void expression()
{
    // TODO(TODD) Express yourself
}

bool compile(const char* source, Chunk* chunk)
{
    scanner_init(source);
    chunk_compiling = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    compiler_end();

    return !parser.had_error;
}


