#include <stdio.h>
#include <curl/curl.h>
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAX_PAIRS 7

struct api_struct {
    char *key;
    char *value;
};
struct param {
    const char *name;
    const char *value;
};
struct response_buffer {
    char *data;
    size_t length;
};
int get_holidays (char *api, const char *country, const int year);

int main(void) {
    get_holidays("https://date.nager.at/api/v3/PublicHolidays/{year}/{country}", "PL", 2026);
}

static int append_to_buffer(char **buffer, size_t *length, const char *text, size_t text_length) {
    char *updated = realloc(*buffer, *length + text_length + 1);
    if (updated == NULL) {
        return 1;
    }
    *buffer = updated;
    memcpy(*buffer + *length, text, text_length);
    *length += text_length;
    (*buffer)[*length] = '\0';
    return 0;
}

char *put_values_into_url(const char *url, ...) {
    va_list args;
    va_start(args, url);
    struct param params[MAX_PAIRS];
    char compare_buffer[32];
    int n = 0;
    const char *name;
    while (n < MAX_PAIRS && (name = va_arg(args, const char *)) != NULL) {
        const char *value = va_arg(args, const char *);
        if (value == NULL) {
            va_end(args);
            return NULL;
        }
        params[n].name = name;
        params[n].value = value;
        n++;
    }
    va_end(args);

    if (url == NULL) {
        return NULL;
    }

    char *buffer = malloc(1);
    if (buffer == NULL) {
        return NULL;
    }
    buffer[0] = '\0';
    size_t written = 0;
    const char *src = url;
    const char *opening;
    while ((opening = strchr(src, '{')) != NULL) {
        const char *closing = strchr(opening + 1, '}');
        if (closing == NULL) {
            free(buffer);
            return NULL;
        }
        size_t name_len = (size_t)(closing - opening - 1);
        if (name_len == 0 || name_len >= sizeof compare_buffer ||
            append_to_buffer(&buffer, &written, src, (size_t)(opening - src)) != 0) {
            free(buffer);
            return NULL;
        }

        memcpy(compare_buffer, opening + 1, name_len);
        compare_buffer[name_len] = '\0';
        const char *value = NULL;
        for (int i = 0; i < n; i++) {
            if (strcmp(params[i].name, compare_buffer) == 0) {
                value = params[i].value;
                break;
            }
        }
        if (value == NULL || append_to_buffer(&buffer, &written, value, strlen(value)) != 0) {
            free(buffer);
            return NULL;
        }
        src = closing + 1;
    }

    if (append_to_buffer(&buffer, &written, src, strlen(src)) != 0) {
        free(buffer);
        return NULL;
    }
    return buffer;

}
size_t write_data (char *buffer, size_t size, size_t nmeb, void *userp) {
    size_t bytes = size * nmeb;
    struct response_buffer *response = userp;

    char *new_data = realloc(response->data, response->length + bytes + 1);
    if (new_data == NULL) {
        return 0;
    }
    response->data = new_data;
    memcpy(response->data + response->length, buffer, bytes);
    response->length += bytes;
    response->data[response->length] = '\0';

    return bytes;
}

int get_holidays (char *api, const char *country, const int year) {
    char year_value[12];
    snprintf(year_value, sizeof year_value, "%d", year);
    api = put_values_into_url(api, "year", year_value, "country", country, NULL);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        return 1;
    }

    char url[128];
    int len = snprintf(url, sizeof url, api, year, country);
    if (len < 0 || (size_t)len >= sizeof url) {
        fprintf(stderr, "URL too long\n");
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    struct response_buffer response = {0};
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl: %s\n", curl_easy_strerror(res));
    } else {
        putchar('\n');
    }

    cJSON *holidays = cJSON_Parse(response.data);
    if (holidays != NULL && cJSON_IsArray(holidays)) {
        int count = cJSON_GetArraySize(holidays);
        for (int i = 0; i < count; i++) {
            cJSON *holiday = cJSON_GetArrayItem(holidays, i);
            cJSON *date = cJSON_GetObjectItem(holiday, "date");

            if (cJSON_IsString(date)) {
                printf("%s\n", date->valuestring);
            }
        }
    }
    cJSON_Delete(holidays);
    free(response.data);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(api);
    return res != CURLE_OK;
}


