//minimal wget tool fueled by curl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

static void filename_from_url(const char *url, char *out, size_t outlen) {
    const char *slash = strrchr(url, '/');
    const char *name = (slash && *(slash + 1) != '\0') ? slash + 1 : "index.html";
    strncpy(out, name, outlen - 1);
    out[outlen - 1] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <url> [-O <output>]\n", argv[0]);
        return 1;
    }

    const char *url = argv[1];
    char output[512];

    if (argc >= 4 && strcmp(argv[2], "-O") == 0) {
        strncpy(output, argv[3], sizeof(output) - 1);
        output[sizeof(output) - 1] = '\0';
    } else {
        filename_from_url(url, output, sizeof(output));
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }

    FILE *fp = fopen(output, "wb");
    if (!fp) {
        perror("fopen");
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ButterOS-wget/1.0");

    printf("Downloading: %s\n", url);
    printf("Saving to: %s\n", output);

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);

    if (res != CURLE_OK) {
        fprintf(stderr, "wget error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        remove(output); 
        return 1;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code >= 400) {
        fprintf(stderr, "wget error: server returned HTTP %ld\n", http_code);
        remove(output);
        return 1;
    }

    printf("Done.\n");
    return 0;
}
