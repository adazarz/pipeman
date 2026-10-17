#include <windows.h>
#include <stdio.h>
// #include <wchar.h>

void get_yaml(char *buffer, size_t size) {
	size = GetModuleFileNameA(NULL, buffer, size - 1);

	char *slash = strrchr(buffer, L'\\');
	if (slash) {
		snprintf(slash + 1, 13, "reports.yaml");
		// *slash = L'reports.yaml\0';
	}
	printf("%s\n", buffer);
}
