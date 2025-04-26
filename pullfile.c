#include <stdio.h>
#include <curl/curl.h>

size_t write_callback(void *contents, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(contents, size, nmemb, stream);
}

int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    if(dltotal > 0){
        double percent = (dltotal > 0) ? (double)dlnow/dltotal*100 : 0;
        printf("Progress: %.2f%% (%" CURL_FORMAT_CURL_OFF_T "/%" CURL_FORMAT_CURL_OFF_T ")\r", percent, dlnow, dltotal);
    }
    return 0;
}

int main(int argc, char **argv) {
    if(argc != 3) {
        printf("Usage: %s <URL> <output_file>\n", argv[0]);
        return 1;
    }

    CURL *curl = NULL;
    FILE *fp = fopen(argv[2], "wb");
    curl_global_init(CURL_GLOBAL_DEFAULT);

    if(!fp || !(curl = curl_easy_init())) {
        fprintf(stderr, "Initialization failed\n");
        goto cleanup;
    }

    curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code >= 400) {
        const char* err_desc = "Unknown error";
        switch(http_code) {
            case 400: err_desc = "Bad Request"; break;
            case 401: err_desc = "Unauthorized"; break;
            case 403: err_desc = "Forbidden"; break;
            case 404: err_desc = "Not Found"; break;
            case 500: err_desc = "Internal Server Error"; break;
        }
        fprintf(stderr, "HTTP Error %ld: %s\n", http_code, err_desc);
    }
    if(res == CURLE_SSL_CONNECT_ERROR) {
        long verify_result;
        curl_easy_getinfo(curl, CURLINFO_SSL_VERIFYRESULT, &verify_result);
        fprintf(stderr, "SSL ERROR: %s (Code:%ld)\n", 
                curl_easy_strerror(res), verify_result);
    }
    if(res != CURLE_OK)
        fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(res));
cleanup:
    if(fp) fclose(fp);
    if(curl) curl_easy_cleanup(curl);
    curl_global_cleanup();
    putc('\n',stdout);
    return 0;
}
