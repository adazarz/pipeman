#include <stdio.h>
#include <curl/curl.h>

int main(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        return 1;
    }

    int year = 2026;
    const char *country = "PL";

    char url[128];
    int len = snprintf(url, sizeof url,
                       "https://date.nager.at/api/v3/PublicHolidays/%d/%s",
                       year, country);
    if (len < 0 || (size_t)len >= sizeof url) {
        fprintf(stderr, "URL za długi\n");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl: %s\n", curl_easy_strerror(res));
    } else {
        putchar('\n');
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return res != CURLE_OK;
}
