/* Hi Dr. Lamb!
 *
 * Compile with:
 * gcc -o hw hw.c -lcurl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

/* buffer for response */
struct MemoryStruct {
    char *memory;
    size_t size;
};

/* callback for response */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        /* womp womp, no memory */
        printf("not enough memory\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

/* help commands */
void print_help() {
    printf("Usage:\n");
    printf("  hw [options] <data>\n");
    printf("Options:\n");
    printf("  -u, --url <url>          Specify the URL\n");
    printf("  -g, --get                Perform HTTP GET\n");
    printf("  -o, --post               Perform HTTP POST\n");
    printf("  -p, --put                Perform HTTP PUT\n");
    printf("  -d, --delete             Perform HTTP DELETE\n");
    printf("  -h, --help               Show this help message\n");
}

/* main */
int main(int argc, char *argv[]) {
    if(argc < 2) {
        print_help();
        return 1;
    }

    char *url = NULL;
    char *data = NULL;
    enum { NONE, GET, POST, PUT, DELETE } method = NONE;

    /* parse args */
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--url") == 0) {
            if(i+1 < argc) {
                url = argv[++i];
            } else {
                fprintf(stderr, "Error: --url requires an argument\n");
                return 1;
            }
        } else if(strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--get") == 0) {
            method = GET;
        } else if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--post") == 0) {
            method = POST;
        } else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--put") == 0) {
            method = PUT;
        } else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--delete") == 0) {
            method = DELETE;
        } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else {
            /* treat as data */
            if(data == NULL) {
                data = argv[i];
            } else {
                /* concatenate  */
                size_t len = strlen(data) + strlen(argv[i]) + 2;
                char *newdata = malloc(len);
                snprintf(newdata, len, "%s %s", data, argv[i]);
                data = newdata;
            }
        }
    }

    if(url == NULL) {
        fprintf(stderr, "Error: URL REQUIRED!\n");
        return 1;
    }

    if(method == NONE) {
        fprintf(stderr, "Error: You must specify an HTTP method\n");
        return 1;
    }

    if((method == POST || method == PUT) && data == NULL) {
        fprintf(stderr, "Error: This method requires data\n");
        return 1;
    }

    CURL *curl;
    CURLcode res;
    long response_code = 0;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        switch(method) {
            case GET:
                /* default */
                break;
            case POST:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
                break;
            case PUT:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
                break;
            case DELETE:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                if(data) {
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
                }
                break;
            default:
                break;
        }

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            free(chunk.memory);
            return 2;
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        printf("%ld %s\n", response_code, chunk.memory);

        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    curl_global_cleanup();

    return 0;
}

