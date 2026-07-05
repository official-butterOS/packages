#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REPO_URL "https://github.com/VxidDev/Arc/archive/refs/heads/main.zip"
#define ZIP_NAME "Arc-main.zip"
#define EXTRACTED_DIR "Arc-main"

static void run_step(const char *description, const char *cmd) {
    printf("==> %s\n", description);
    int status = system(cmd);
    if (status != 0) {
        fprintf(stderr, "FAILED: %s (command: %s)\n", description, cmd);
        exit(1);
    }
}

int main(void) {
    char cmd[1024];


    snprintf(cmd, sizeof(cmd), "curl -L -o %s %s", ZIP_NAME, REPO_URL);
    run_step("Downloading Arc from GitHub", cmd);

    snprintf(cmd, sizeof(cmd), "unzip -o %s", ZIP_NAME);
    run_step("Extracting archive", cmd);


    snprintf(cmd, sizeof(cmd),
             "cd %s && make", EXTRACTED_DIR);
    run_step("Running make", cmd);

    snprintf(cmd, sizeof(cmd),
             "cd %s && make release", EXTRACTED_DIR);
    run_step("Running make release", cmd);

    snprintf(cmd, sizeof(cmd),
             "cd %s && sudo make install", EXTRACTED_DIR);
    run_step("Running sudo make install", cmd);

    snprintf(cmd, sizeof(cmd),
             "cd %s && sudo make install-libs", EXTRACTED_DIR);
    run_step("Running sudo make install-libs", cmd);


    snprintf(cmd, sizeof(cmd), "rm -f %s", ZIP_NAME);
    system(cmd);

    printf("==> Arc installed successfully.\n");
    return 0;
}
