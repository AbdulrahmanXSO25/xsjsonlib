#include "jsonhelper.h"
#include <string.h>   // For memcpy, strlen
#include <stdlib.h>   // For malloc, free
#include <ctype.h>    // For isspace

static int globalLine = 1;
static int globalColumn = 1;

char *_strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *new_str = (char *)malloc(len + 1);
    if (new_str) {
        memcpy(new_str, s, len);
        new_str[len] = '\0';
    }
    return new_str;
}

void initObject(JsonValue *obj) {
    obj->type = JSON_OBJECT;
    obj->data.objectValue = malloc(sizeof(object));
    if (obj->data.objectValue == NULL) {
        fprintf(stderr, "Failed to allocate memory for object.\n");
        exit(EXIT_FAILURE);
    }
    obj->data.objectValue->count = 0;
    obj->data.objectValue->capacity = 4;
    obj->data.objectValue->pairs = malloc(obj->data.objectValue->capacity * sizeof(pair));
    if (obj->data.objectValue->pairs == NULL) {
        fprintf(stderr, "Failed to allocate memory for object pairs.\n");
        exit(EXIT_FAILURE);
    }
}

void initArray(JsonValue *arr) {
    arr->type = JSON_ARRAY;
    arr->data.arrayValue = malloc(sizeof(array));
    if (arr->data.arrayValue == NULL) {
        fprintf(stderr, "Failed to allocate memory for array.\n");
        exit(EXIT_FAILURE);
    }
    arr->data.arrayValue->length = 0;
    arr->data.arrayValue->capacity = 4;
    arr->data.arrayValue->items = malloc(arr->data.arrayValue->capacity * sizeof(JsonValue));
    if (arr->data.arrayValue->items == NULL) {
        fprintf(stderr, "Failed to allocate memory for array items.\n");
        exit(EXIT_FAILURE);
    }
}

void addObjectItem(JsonValue *obj, const char *key, JsonValue *value) {
    if (obj->data.objectValue->count >= obj->data.objectValue->capacity) {
        obj->data.objectValue->capacity *= 2;
        obj->data.objectValue->pairs = realloc(obj->data.objectValue->pairs, obj->data.objectValue->capacity * sizeof(pair));
        if (obj->data.objectValue->pairs == NULL) {
            fprintf(stderr, "Failed to reallocate memory for object pairs.\n");
            exit(EXIT_FAILURE);
        }
    }
    obj->data.objectValue->pairs[obj->data.objectValue->count].key = strdup(key);
    obj->data.objectValue->pairs[obj->data.objectValue->count].value = value;
    obj->data.objectValue->count++;
}

void addArrayItem(JsonValue *arr, JsonValue *item) {
    if (arr->data.arrayValue->length >= arr->data.arrayValue->capacity) {
        arr->data.arrayValue->capacity *= 2;
        arr->data.arrayValue->items = realloc(arr->data.arrayValue->items, arr->data.arrayValue->capacity * sizeof(JsonValue));
        if (arr->data.arrayValue->items == NULL) {
            fprintf(stderr, "Failed to reallocate memory for array items.\n");
            exit(EXIT_FAILURE);
        }
    }
    arr->data.arrayValue->items[arr->data.arrayValue->length++] = *item;
}

void json_free(JsonValue *value) {
    if (!value) return;

    switch (value->type) {
        case JSON_STRING:
            free(value->data.stringValue);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < value->data.objectValue->count; i++) {
                free(value->data.objectValue->pairs[i].key);
                json_free(value->data.objectValue->pairs[i].value);
            }
            free(value->data.objectValue->pairs);
            free(value->data.objectValue);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < value->data.arrayValue->length; i++) {
                json_free(&value->data.arrayValue->items[i]);
            }
            free(value->data.arrayValue->items);
            free(value->data.arrayValue);
            break;
        default:
            break;
    }

    free(value);
}

void printJsonToFile(FILE *file, JsonValue *value);

void printJsonObjectToFile(FILE *file, JsonValue *value) {
    fprintf(file, "{");
    for (size_t i = 0; i < value->data.objectValue->count; ++i) {
        fprintf(file, "\"%s\":", value->data.objectValue->pairs[i].key);
        printJsonToFile(file, value->data.objectValue->pairs[i].value);
        if (i < value->data.objectValue->count - 1) {
            fprintf(file, ",");
        }
    }
    fprintf(file, "}");
}

void printJsonArrayToFile(FILE *file, JsonValue *value) {
    fprintf(file, "[");
    for (size_t i = 0; i < value->data.arrayValue->length; ++i) {
        printJsonToFile(file, value->data.arrayValue->items + i);
        if (i < value->data.arrayValue->length - 1) {
            fprintf(file, ",");
        }
    }
    fprintf(file, "]");
}

void printJsonToFile(FILE *file, JsonValue *value) {
    if (!value) return;

    switch (value->type) {
        case JSON_INT:
            fprintf(file, "%d", value->data.intValue);
            break;
        case JSON_FLOAT:
            fprintf(file, "%f", value->data.floatValue);
            break;
        case JSON_STRING:
            fprintf(file, "\"%s\"", value->data.stringValue);
            break;
        case JSON_OBJECT:
            printJsonObjectToFile(file, value);
            break;
        case JSON_ARRAY:
            printJsonArrayToFile(file, value);
            break;
        case JSON_BOOL:
            fprintf(file, "%s", value->data.boolValue ? "true" : "false");
            break;
        case JSON_NULL:
            fprintf(file, "null");
            break;
        default:
            fprintf(file, "null");
    }
}

void skipWhitespaceAndNewLines(const char **json) {
    while(isspace(**json)) {
        if (**json == '\n') {
            globalLine++;
            globalColumn = 1;
        } else {
            globalColumn++;
        }
        (*json)++;
    }
}

Token getNextToken(const char **json) {
    Token token;
    memset(&token, 0, sizeof(Token));
    skipWhitespaceAndNewLines(json);

    switch (**json) {
        case '\0': token.type = TOKEN_EOF; break;
        case '{': token.type = TOKEN_LBRACE; (*json)++; break;
        case '}': token.type = TOKEN_RBRACE; (*json)++; break;
        case '[': token.type = TOKEN_LBRACKET; (*json)++; break;
        case ']': token.type = TOKEN_RBRACKET; (*json)++; break;
        case ',': token.type = TOKEN_COMMA; (*json)++; break;
        case ':': token.type = TOKEN_COLON; (*json)++; break;
        case '"':
            (*json)++;
            const char *start = *json;
            while (**json && **json != '"') {
                if (**json == '\\') (*json)++;
                (*json)++;
            }
            token.type = TOKEN_STRING;
            token.value = _strndup(start, *json - start);
            (*json)++;
            break;
        default:
            if (isdigit(**json) || **json == '-') {
                start = *json;
                while (isdigit(*++(*json)));
                if (**json == '.') while (isdigit(*++(*json)));
                token.type = TOKEN_NUMBER;
                token.value = _strndup(start, *json - start);
            } else {
                start = *json;
                while (isalpha(*++(*json)));
                token.value = _strndup(start, *json - start);
                if (strcmp(token.value, "true") == 0) token.type = TOKEN_TRUE;
                else if (strcmp(token.value, "false") == 0) token.type = TOKEN_FALSE;
                else if (strcmp(token.value, "null") == 0) token.type = TOKEN_NULL;
                else {
                    fprintf(stderr, "Unexpected token '%s' at line %d, column %d\n",
                            token.value, globalLine, globalColumn);
                    free(token.value);
                    token.type = TOKEN_ERROR;
                    return token;
                }
            }
            break;
    }

    return token;
}

void expectToken(TokenType expected, const char **json) {
    Token token = getNextToken(json);
    if (token.type != expected) {
        fprintf(stderr, "JSON parse error: expected %d but got %d\n", expected, token.type);
        exit(EXIT_FAILURE);
    }
}

JsonValue *createJsonValue(Token *token) {
    JsonValue *value = malloc(sizeof(JsonValue));
    if (!value) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    switch (token->type) {
        case TOKEN_NUMBER:
            if (strchr(token->value, '.') != NULL) { // Check if it contains a decimal point
                value->type = JSON_FLOAT;
                value->data.floatValue = atof(token->value);
            } else {
                value->type = JSON_INT;
                value->data.intValue = atoi(token->value);
            }
            break;
        
        case TOKEN_STRING:
            value->type = JSON_STRING;
            value->data.stringValue = strdup(token->value);
            if (!value->data.stringValue) {
                free(value);
                fprintf(stderr, "Memory allocation failed for string\n");
                return NULL;
            }
            break;

        case TOKEN_TRUE:
            value->type = JSON_BOOL;
            value->data.boolValue = true;
            break;

        case TOKEN_FALSE:
            value->type = JSON_BOOL;
            value->data.boolValue = false;
            break;

        case TOKEN_NULL:
            value->type = JSON_NULL;
            break;

        default:
            free(value);
            fprintf(stderr, "Invalid token type for value creation\n");
            return NULL;
    }

    return value;
}

JsonValue *parseObject(const char **json) {
    JsonValue *obj = malloc(sizeof(JsonValue));
    if (!obj) return NULL;
    obj->type = JSON_OBJECT;
    initObject(obj);

    while (1) {
        Token keyToken = getNextToken(json);
        if (keyToken.type == TOKEN_RBRACE) {
            break; // End of object
        }
        if (keyToken.type != TOKEN_STRING) {
            fprintf(stderr, "Expected string key but found '%s'\n", keyToken.value);
            json_free(obj);
            return NULL;
        }

        expectToken(TOKEN_COLON, json); // Checks for colon and moves the position
        JsonValue *subValue = parseJson(json);
        addObjectItem(obj, keyToken.value, subValue);
        free(keyToken.value);

        Token commaOrEnd = getNextToken(json);
        if (commaOrEnd.type == TOKEN_RBRACE) {
            break; // Properly end the object
        }
        if (commaOrEnd.type != TOKEN_COMMA) {
            fprintf(stderr, "Expected ',' or '}' but found '%s'\n", commaOrEnd.value);
            json_free(obj);
            return NULL;
        }
    }
    return obj;
}

JsonValue *parseArray(const char **json) {
    JsonValue *arr = malloc(sizeof(JsonValue));
    if (!arr) return NULL;
    arr->type = JSON_ARRAY;
    initArray(arr); // Assuming this function initializes a JSON array

    while (1) {
        Token nextToken = getNextToken(json);
        if (nextToken.type == TOKEN_RBRACKET) {
            break; // End of array
        }
        (*json)--; // Adjust reading position if necessary
        JsonValue *item = parseJson(json);
        addArrayItem(arr, item);
        nextToken = getNextToken(json);
        if (nextToken.type == TOKEN_RBRACKET) {
            break; // End of array
        }
        if (nextToken.type != TOKEN_COMMA) {
            fprintf(stderr, "Expected ',' or ']' but found '%s'\n", nextToken.value);
            json_free(arr);
            return NULL;
        }
    }
    return arr;
}

JsonValue *parseJson(const char **json) {
    Token token = getNextToken(json);
    JsonValue *value = NULL;

    switch (token.type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NULL:
            value = createJsonValue(&token);
            break;
        case TOKEN_LBRACE:
            value = parseObject(json);
            break;
        case TOKEN_LBRACKET:
            value = parseArray(json);
            break;
        default:
            fprintf(stderr, "Unexpected token '%s' at line %d, column %d\n",
                token.value ? token.value : "<none>", globalLine, globalColumn);
            if (value) json_free(value);
            return NULL;
    }
    return value;
}