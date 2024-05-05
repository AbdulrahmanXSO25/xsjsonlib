#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <stddef.h>  // For size_t
#include <stdio.h>   // For FILE
#include <stdbool.h> // For boolean types in C99+

typedef struct JsonValue JsonValue;

typedef struct {
    char *key;
    JsonValue *value;
} pair;

typedef struct {
    size_t count;
    size_t capacity;
    pair *pairs;
} object;

typedef struct {
    size_t length;
    size_t capacity;
    JsonValue *items;
} array;

typedef enum {
    JSON_NULL,
    JSON_INT,
    JSON_FLOAT,
    JSON_STRING,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_BOOL
} JsonType;

typedef union {
    int intValue;
    float floatValue;
    char *stringValue;
    bool boolValue;
    object *objectValue;
    array *arrayValue;
} JsonValueData;

struct JsonValue {
    JsonType type;
    JsonValueData data;
};

typedef enum {
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_LBRACE,  // {
    TOKEN_RBRACE,  // }
    TOKEN_LBRACKET,// [
    TOKEN_RBRACKET,// ]
    TOKEN_COLON,   // :
    TOKEN_COMMA,   // ,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

void initObject(JsonValue *obj);
void initArray(JsonValue *arr);
void addObjectItem(JsonValue *obj, const char *key, JsonValue *value);
void addArrayItem(JsonValue *arr, JsonValue *item);
void json_free(JsonValue *value);

void printJsonToFile(FILE *file, JsonValue *value);
void printJsonObjectToFile(FILE *file, JsonValue *value);
void printJsonArrayToFile(FILE *file, JsonValue *value);

void skipWhitespaceAndNewLines(const char **json);
Token getNextToken(const char **json);
void expectToken(TokenType expected, const char **json);

JsonValue *parseJson(const char **json);
JsonValue *parseObject(const char **json);
JsonValue *parseArray(const char **json);
JsonValue *createJsonValue(Token *token);

#endif // JSONHELPER_H