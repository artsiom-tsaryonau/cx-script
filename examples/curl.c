#!/usr/bin/env cx
//DEPS vcpkg:curl AS CURL::libcurl
#include <stdio.h>
#include <curl/curl.h>

int main(int argc, char* argv[]) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }

    printf("Executing HTTP request from: %s\n", argv[1]);

    curl_easy_setopt(curl, CURLOPT_URL, "https://httpbin.org/user-agent");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return 0;
}