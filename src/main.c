#include <stdio.h>
#include <stdlib.h>
#include "jsonhelper.h"

int main() {

    FILE *fd = fopen("../example.json", "r");
    
    if(!fd){
        perror("error open example.json");
        return EXIT_FAILURE;
    }

    char json_example[10000];

    size_t num_read = fread(json_example, sizeof(char), sizeof(json_example), fd);

    fread(json_example, 10000, 10000, fd);

    fclose(fd);

    json_example[num_read] = '\0';

    const char* pointer_to_json = json_example;
    JsonValue* root = parseJson(&pointer_to_json);

    FILE *file = fopen("output.json", "w");
    if (!file) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    printJsonToFile(file, root);

    fclose(file);

    json_free(root);

    return 0;
}