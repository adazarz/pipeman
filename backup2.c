#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include <yaml.h>
#include <time.h>
#include <ctype.h>
int due_in[4];

struct report {
    char name[50];
    char **input;
    int input_count;
    char output[300];
    int output_count;
    char script[300];
    char **dependencies;
    size_t dependency_count;
    struct tm due;
    struct tm due_day;
    int due_in[4];
};

int get_reports_from_yaml(char *reports, struct report **pipelines, size_t *reports_count);
void print_pipeline(struct report *p);
int get_date(char *date, struct report *report, struct tm *out_due_date);
struct tm get_due_day(struct tm due, int wd);
void print_date(struct tm date);
void free_reports(struct report *reports, size_t n);

int main(int argc, char *argv[]) {
    size_t reports_count = 0;
    char reports[4096];
    struct report *pipelines = NULL;
    int k;

    get_yaml(reports, sizeof(reports));
    if (get_reports_from_yaml(reports, &pipelines, &reports_count) != 0) {
        free_reports(pipelines, reports_count);
        return 1;
    }

    printf("Number of reports overall: %d\n", reports_count);
    for (k = 0; k < reports_count; k++) {
        print_pipeline(&pipelines[k]);
    }

    // pid_t t = fork();
    if (argc >= 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Pipeman (shorthand for report manager) is a simple yet flexible tool for orchestrating periodic scripts and reports. Configuration "
            "is done in \"reports.yaml\" file.\n\nCommands:\n-h or --help          \tDisplay this help message\nrun                   \tRun all due pipelines\n"
            "info                    Display information about all pipelines, their due dates and statuses\nexecute [report names]\tExecute particular report/-s\n");
        } else if (strcmp(argv[1], "info") == 0) {
            //Info implementation

        } else if (strcmp(argv[1], "run") == 0) {
            //Run implementation
        } else if (strcmp(argv[1], "execute") == 0) {
            //Execute implementation
        }
    }

    free_reports(pipelines, reports_count);
    return 0;
}

void free_reports(struct report *reports, size_t n) {
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < reports[i].dependency_count; j++) {
            free(reports[i].dependencies[j]);
        }
        free(reports[i].dependencies);
    }
    free(reports);
}

int get_reports_from_yaml(char *reports, struct report **pipelines, size_t *reports_count) {
    yaml_parser_t parser;
    yaml_document_t document;
    if (yaml_parser_initialize(&parser) == 0) {
        printf("Error initializing a parser\n");
        return 1;
    }

    FILE *fp = fopen(reports, "r");
    if (fp == NULL) {
        printf("Error opening the reports.yaml");
        return 1;
    }
    yaml_parser_set_input_file(&parser, fp);

    yaml_parser_load(&parser, &document);

    yaml_node_t *root = yaml_document_get_root_node(&document);
    for (yaml_node_pair_t *pair = root->data.mapping.pairs.start; pair < root->data.mapping.pairs.top; pair++) {
        yaml_node_t *key = yaml_document_get_node(&document, pair->key);
        yaml_node_t *value = yaml_document_get_node(&document, pair->value);

        if (key->type == YAML_SCALAR_NODE && strcmp((char *)key->data.scalar.value, "pipelines") == 0 && value->type == YAML_MAPPING_NODE) {
            size_t n_pipelines = value->data.mapping.pairs.top - value->data.mapping.pairs.start;
            *pipelines = calloc(n_pipelines, sizeof **pipelines);
            // printf("top %zu\n", n_pipelines);
            int n = 0;
            for (yaml_node_pair_t *pipeline_pair = value->data.mapping.pairs.start; pipeline_pair < value->data.mapping.pairs.top; pipeline_pair++) {
                yaml_node_t *pipeline_name = yaml_document_get_node(&document, pipeline_pair->key);
                yaml_node_t *pipeline_data = yaml_document_get_node(&document, pipeline_pair->value);

                if (pipeline_name->type == YAML_SCALAR_NODE) {
                    (*reports_count)++;
                    snprintf((*pipelines)[n].name, sizeof((*pipelines)[n].name), "%s", pipeline_name->data.scalar.value);
                    if (pipeline_data->type == YAML_MAPPING_NODE) {
                        for (yaml_node_pair_t *data_pair = pipeline_data->data.mapping.pairs.start; data_pair < pipeline_data->data.mapping.pairs.top; data_pair++) {
                            yaml_node_t *field_name = yaml_document_get_node(&document, data_pair->key);
                            yaml_node_t *field_value = yaml_document_get_node(&document, data_pair->value);
                            char *name = (char *)field_name->data.scalar.value;
                            if (strcmp(name, "input") == 0) {
                                int input_count = field_value->data.sequence.items.top - field_value->data.sequence.items.start;
                                (*pipelines)[n].input = malloc(input_count * sizeof *(*pipelines)[n].input);
                                (*pipelines)[n].input_count = input_count;
                                int b = 0;
                                for (yaml_node_item_t *item = field_value->data.sequence.items.start; item < field_value->data.sequence.items.top; item++) {
                                    yaml_node_t *input = yaml_document_get_node(&document, *item);
                                    const char *inp = (const char *)input->data.scalar.value;
                                    (*pipelines)[n].input[b] = malloc(strlen(inp) + 1);
                                    strcpy((*pipelines)[n].input[b], inp);
                                    b++;
                                }
                                // snprintf((*pipelines)[n].input, sizeof((*pipelines)[n].input), "%s", field_value->data.scalar.value);
                            } else if (strcmp(name, "output") == 0) {
                                snprintf((*pipelines)[n].output, sizeof((*pipelines)[n].output), "%s", field_value->data.scalar.value);
                            } else if (strcmp(name, "script") == 0) {
                                snprintf((*pipelines)[n].script, sizeof((*pipelines)[n].script), "%s", field_value->data.scalar.value);
                            } else if (strcmp(name, "due") == 0) {
                                struct tm due;
                                if (get_date(field_value->data.scalar.value, &(*pipelines)[n], &due) != 0) {
                                    return 1;
                                }
                                (*pipelines)[n].due = due;
                            } else if (strcmp(name, "dependencies") == 0) {
                                size_t dependency_count = field_value->data.sequence.items.top - field_value->data.sequence.items.start;
                                printf("dependency count %zu\n", dependency_count);
                                (*pipelines)[n].dependencies = malloc(dependency_count * sizeof *(*pipelines)[n].dependencies);
                                (*pipelines)[n].dependency_count = dependency_count;
                                int m = 0;
                                for (yaml_node_item_t *item = field_value->data.sequence.items.start; item < field_value->data.sequence.items.top; item++) {
                                    yaml_node_t *dependency = yaml_document_get_node(&document, *item);
                                    const char *dep = (const char *)dependency->data.scalar.value;
                                     (*pipelines)[n].dependencies[m] = malloc(strlen(dep) + 1);
                                         if ((*pipelines)[n].dependencies[m] == NULL) {
                                            printf("%s\n", "Error allocating memory");
                                            return 1;
                                        }
                                     strcpy((*pipelines)[n].dependencies[m], dep);
                                    // snprintf(pipelines[n].dependencies[m], sizeof(pipelines[n].dependencies[m]), "%s", dependency->data.scalar.value);
                                    m++;
                                }
                            }
                        }
                    } else {
                        printf("Malformed yaml file\n");
                        return 1;
                    }

                } else {
                    printf("Malformed yaml file\n");
                    return 1;
                }
                n++;   
            }
    }}
    return 0;
}

struct tm get_due_day(struct tm due, int wd) {
    // print_date(due);
    //minute hour day month
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    // struct tm due_day = *localtime(&now);
    int dif;
    int i;
    for (i=0; i<4; i++) {
        due_in[i] = 0;
    }
    int requested_dow = due.tm_wday;
    int dow_specified = (requested_dow != -1); // -1 = unspecified

    // -1 for unspecified month because 0 == Jan
    due.tm_year = local->tm_year;
    if (due.tm_mon != -1) {
        if ((dif = (due.tm_mon - local->tm_mon)) < 0) {
            due.tm_year = local->tm_year + 1;
        }
        if (due.tm_mday == 0) due.tm_mday = 1;
        if (due.tm_hour == -1) due.tm_hour = 0;
        if (due.tm_min == -1) due.tm_min = 0;
    } else {
        if (due.tm_mday != 0) {
            if ((dif = (due.tm_mday - local->tm_mday)) < 0) {
                due.tm_mon = local->tm_mon + 1;
                if (due.tm_hour == -1) due.tm_hour = 0;
                if (due.tm_min == -1) due.tm_min = 0;
            }
        } else {
            due.tm_mon = local->tm_mon;
            if (due.tm_hour != -1) {
                if ((dif = (due.tm_hour - local->tm_hour)) < 0) {
                    due.tm_mday = local->tm_mday + 1;
                }
                if (due.tm_min == -1) due.tm_min = 0;
            } else {
                due.tm_mday = local->tm_mday;
                due.tm_hour = 0;
                if (due.tm_min != -1) {
                    if ((dif = (due.tm_min - local->tm_min)) < 0) {
                        due.tm_hour = local->tm_hour + 1;
                    }
                } else {
                    due.tm_min = 0;
                }
            }
        }
    }


    // print_date(due);
    // print_date(*local);
    time_t t_due = mktime(&due);
    // print_date(due);
    time_t t_local = mktime(local);
    double sum = difftime(t_due, t_local);
    // printf("sum %.0f\n", sum);

    // Calculating for wd+n
    if (wd == 1) {
        int outcome = 2 * due.tm_mday / 7;
        int rest = due.tm_mday % 7;
        // printf("wday %d\n", due.tm_wday);
        int weekd_first_monthd = due.tm_wday - rest + 1;
        for (i = 1; i <= rest; i++, weekd_first_monthd++) {
            if (weekd_first_monthd % 7 == 0 || weekd_first_monthd % 7 == 6) {
                outcome += 1;
                rest++;
            }
        }
        due.tm_mday += outcome;
        due_in[2] += outcome;
        t_due = mktime(&due);
    }
    // Calculating next due day by the day-of-week argument
    if (dow_specified) {
        int months_length[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        struct tm dow_date = due;
        int l;
        for (l = local->tm_mday; l < months_length[dow_date.tm_mon]; l++) {
            dow_date.tm_mday = l;
            time_t t_dow_date = mktime(&dow_date);
            double e;
            if ((dow_date.tm_wday == requested_dow) && (e = difftime(t_dow_date, t_due)) < 0) {
                due = dow_date;
                break;
            }
        }
    }
    return due;
}

int get_date(char *date, struct report *report, struct tm *out_due_date) {
    int i = 0;
    int wd = 0;
    int count = 0;
    int pos = 0;
    char buffer[5];
    if (tolower(date[0]) == 'w' && tolower(date[1]) == 'd') {
        //Working day date
        wd = 1;
        date = &date[3];
    }
    struct tm due_date = {0};
    due_date.tm_isdst = -1;
    do {
        if ((date[i] != ' ') && (date[i] != '\0')) {
            buffer[pos] = date[i];
            pos++;
            buffer[pos] = '\0';
        } else {
            switch (count) {
                case 0:
                    due_date.tm_min = (strcmp(buffer, "*") == 0) ? -1 : (int)strtol(buffer, NULL, 10); // In struct tm minutes range 0-59
                    break;
                case 1:
                    due_date.tm_hour = (strcmp(buffer, "*") == 0) ? -1 : (int)strtol(buffer, NULL, 10); // In struct tm hours range 0-23
                    break;
                case 2:
                    due_date.tm_mday = (strcmp(buffer, "*") == 0) ? 0 : (int)strtol(buffer, NULL, 10); // 1-31
                    break;
                case 3:
                    if (buffer == 0) {
                        printf("%s\n", "Unsupported month 0, use integers 1 through 12");
                        return 1;
                    }
                    due_date.tm_mon = (strcmp(buffer, "*") == 0) ? -1 : (int)strtol(buffer, NULL, 10) - 1; //Accounting for c cron format incompatibility
                    break;
                case 4:
                    due_date.tm_wday = (strcmp(buffer, "*") == 0) ? -1 : (int)strtol(buffer, NULL, 10); // 0-6
                    break;
            }
            buffer[0] = '\0';
            count++;
            pos = 0;
            if (date[i] == '\0') {
            break;
        }
        }
        i++;
    } while (1);
    report->due_day = get_due_day(due_date,  wd);
    for (i=0; i < 4; i++) {
        report->due_in[i] = due_in[i];
    }
    *out_due_date = due_date;
    return 0;
}
void change_minus_one_to_stars(char *cron) {
    int i = 0;
    while (cron[i] != '\n' && cron[i] != '\0') {
        if (cron[i] == '-' && cron[i+1] == '1') {
            cron[i] = '*';
            memmove(&cron[i+1], &cron[i+2], strlen(&cron[i+2]) + 1);
        }
        i++;
    }
}
void print_pipeline(struct report *p) {
    char due_print[20];
    char due_day_print[20];
    strftime(due_print, 20, "%M %H %d %m %w", &p->due);
    change_minus_one_to_stars(due_print);
    strftime(due_day_print, 20, "%d.%m.%Y %H:%M", &p->due_day);
    printf("\nname: %s\n", p->name);
    printf("output: %s\n", p->output);
    printf("script: %s\n", p->script);
    printf("dependencies:");
    if (p->dependency_count > 0) {
        printf(" %s\n", p->dependencies[0]);
        for (size_t j = 1; j < p->dependency_count; j++) {
            printf("             %s\n", p->dependencies[j]);
        }
    } else {
        printf("\n");
    }
    printf("input:");
    if (p->input_count > 0) {
        printf(" %s\n", p->input[0]);
        for (int g = 1; g < p->input_count; g++) {
            printf("       %s\n", p->input[g]);
        }
    } else {
        printf("\n");
    }
    printf("due: %s\n", due_print);
    printf("due day: %s\n", due_day_print);
}
void print_date(struct tm date) {
    char dprint[30];
    strftime(dprint, 30, "%d %m %Y %H:%M", &date);
    printf("%s\n", dprint);
}