#!/usr/bin/env cx
//DEPS conan:cjson/1.7.18
#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

int main(int argc, char* argv[]) {
    const char* caller_pwd = argv[1];
    
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/config.json", caller_pwd);

    FILE *f = fopen(config_path, "rb");
    if (!f) {
        printf("No config.json found at %s. Creating sample JSON object...\n", config_path);
        
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "runner", "cx");
        cJSON_AddNumberToObject(root, "version", 1.0);

        char *json_str = cJSON_Print(root);
        printf("%s\n", json_str);

        free(json_str);
        cJSON_Delete(root);
        return 0;
    }

    fclose(f);
    return 0;
}