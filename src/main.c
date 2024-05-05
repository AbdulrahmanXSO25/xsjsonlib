#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define _POSIX_C_SOURCE 200809L

typedef enum {
    JSON_NULL,
    JSON_INT,
    JSON_FLOAT,
    JSON_STRING,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_BOOL
} JsonType;

// AS strndup is available in POSIX envs
char *_strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *new_str = (char *)malloc(len + 1);
    if (new_str) {
        memcpy(new_str, s, len);
        new_str[len] = '\0';
    }
    return new_str;
}

typedef struct JsonValue {
    JsonType type;
    union {
        int intValue;
        float floatValue;
        char *stringValue;
        struct {
            size_t count;
            size_t capacity;
            struct {
                char *key;
                struct JsonValue *value;
            } *pairs;
        } object;
        struct {
            size_t length;
            size_t capacity;
            struct JsonValue *items;
        } array;
        int boolValue;
    } data;
} JsonValue;

void initObject(JsonValue *obj) {
    obj->type = JSON_OBJECT;
    obj->data.object.count = 0;
    obj->data.object.capacity = 4;
    obj->data.object.pairs = malloc(obj->data.object.capacity * sizeof(*(obj->data.object.pairs)));
    if (obj->data.object.pairs == NULL) {
        fprintf(stderr, "Failed to allocate memory for object.\n");
        exit(EXIT_FAILURE);
    }
}

void initArray(JsonValue *arr) {
    arr->type = JSON_ARRAY;
    arr->data.array.length = 0;
    arr->data.array.capacity = 4;
    arr->data.array.items = malloc(arr->data.array.capacity * sizeof(*(arr->data.array.items)));
    if (arr->data.array.items == NULL) {
        fprintf(stderr, "Failed to allocate memory for array.\n");
        exit(EXIT_FAILURE);
    }
}

void addObjectItem(JsonValue *obj, const char *key, JsonValue *value) {
    if (obj->data.object.count >= obj->data.object.capacity) {
        obj->data.object.capacity *= 2;
        obj->data.object.pairs = realloc(obj->data.object.pairs, obj->data.object.capacity * sizeof(*(obj->data.object.pairs)));
        if (obj->data.object.pairs == NULL) {
            fprintf(stderr, "Failed to reallocate memory for object.\n");
            exit(EXIT_FAILURE);
        }
    }
    obj->data.object.pairs[obj->data.object.count].key = strdup(key);
    obj->data.object.pairs[obj->data.object.count].value = value;
    obj->data.object.count++;
}

void addArrayItem(JsonValue *arr, JsonValue *item) {
    if (arr->data.array.length >= arr->data.array.capacity) {
        arr->data.array.capacity *= 2;
        arr->data.array.items = realloc(arr->data.array.items, arr->data.array.capacity * sizeof(*(arr->data.array.items)));
        if (arr->data.array.items == NULL) {
            fprintf(stderr, "Failed to reallocate memory for array.\n");
            exit(EXIT_FAILURE);
        }
    }
    arr->data.array.items[arr->data.array.length++] = *item;
}

void json_free(JsonValue *value) {
    if (!value) return;

    switch (value->type) {
        case JSON_STRING:
            free(value->data.stringValue);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < value->data.object.count; i++) {
                free(value->data.object.pairs[i].key);
                json_free(value->data.object.pairs[i].value);
                free(value->data.object.pairs[i].value);
            }
            free(value->data.object.pairs);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < value->data.array.length; i++) {
                json_free(&(value->data.array.items[i]));
            }
            free(value->data.array.items);
            break;
        default:
            break;
    }

    free(value);
}

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
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

Token getNextToken(const char **json) {
    while (isspace(**json)) (*json)++;

    Token token;
    token.value = NULL;

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
            start = *json;
            while (**json && !isspace(**json) && strchr(",}]", **json) == NULL) (*json)++;
            token.value = _strndup(start, *json - start);
            if (strcmp(token.value, "true") == 0) token.type = TOKEN_TRUE;
            else if (strcmp(token.value, "false") == 0) token.type = TOKEN_FALSE;
            else if (strcmp(token.value, "null") == 0) token.type = TOKEN_NULL;
            else token.type = TOKEN_NUMBER;
            break;
    }

    return token;
}

JsonValue *parseJson(const char **json) {
    Token token = getNextToken(json);
    JsonValue *value = malloc(sizeof(JsonValue));

    if (!value) return NULL;

    switch (token.type) {
        case TOKEN_NUMBER:
            value->type = JSON_INT;
            value->data.intValue = atoi(token.value);
            break;
        case TOKEN_STRING:
            value->type = JSON_STRING;
            value->data.stringValue = token.value;
            break;
        default:
            free(value);
            return NULL;
    }

    return value;
}

void printJsonToFile(FILE *file, JsonValue *value);

void printJsonObjectToFile(FILE *file, JsonValue *value) {
    fprintf(file, "{");
    for (size_t i = 0; i < value->data.object.count; ++i) {
        fprintf(file, "\"%s\":", value->data.object.pairs[i].key);
        printJsonToFile(file, value->data.object.pairs[i].value);
        if (i < value->data.object.count - 1)
            fprintf(file, ",");
    }
    fprintf(file, "}");
}

void printJsonArrayToFile(FILE *file, JsonValue *value) {
    fprintf(file, "[");
    for (size_t i = 0; i < value->data.array.length; ++i) {
        printJsonToFile(file, &value->data.array.items[i]);
        if (i < value->data.array.length - 1)
            fprintf(file, ",");
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

int main() {
    JsonValue root;
    root.type = JSON_OBJECT;
    initObject(&root);
    JsonValue intValue = {.type = JSON_INT, .data.intValue = 123};
    addObjectItem(&root, "exampleNumber", &intValue);
    
    FILE *file = fopen("output.json", "w");
    if (!file) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    printJsonToFile(file, &root);

    fclose(file);

    json_free(&root);

    return 0;
}