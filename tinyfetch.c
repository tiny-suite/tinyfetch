#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#define LABEL "\033[1;34m"
#define VALUE "\033[0m"
#define BOLD  "\033[1;36m"
#define RESET "\033[0m"

static const char *logo[] = {
    " _   _",
    "| |_(_)_ __  _   _",
    "| __| | '_ \\| | | |",
    "| |_| | | | | |_| |",
    " \\__|_|_| |_|\\__, |",
    "             |___/",
};

static int probe(const char *path, const char *key, char *out, size_t size)
{
    FILE *file = fopen(path, "r");
    if (!file)
        return 0;

    char line[512];
    size_t keylen = strlen(key);
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, key, keylen) != 0)
            continue;

        char *sep = strpbrk(line, ":=");
        if (!sep)
            continue;

        char *value = sep + 1;
        while (*value == ' ' || *value == '\t')
            value++;

        int len = (int)strlen(value);
        while (len > 0 && (value[len - 1] == '\n' || value[len - 1] == '\r' ||
                           value[len - 1] == ' ')) {
            value[--len] = '\0';
        }

        if (len >= 2 && value[0] == '"' && value[len - 1] == '"') {
            value++;
            len -= 2;
            value[len] = '\0';
        }

        snprintf(out, size, "%s", value);
        found = 1;
        break;
    }

    fclose(file);
    return found;
}

static long meminfo_kb(const char *key)
{
    FILE *file = fopen("/proc/meminfo", "r");
    if (!file)
        return 0;

    char line[256];
    long value = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            sscanf(line + strlen(key), ": %ld", &value);
            break;
        }
    }

    fclose(file);
    return value;
}

static void format_uptime(char *out, size_t size)
{
    FILE *file = fopen("/proc/uptime", "r");
    if (!file) {
        snprintf(out, size, "unknown");
        return;
    }

    double seconds = 0;
    if (fscanf(file, "%lf", &seconds) != 1)
        seconds = 0;
    fclose(file);

    long total = (long)seconds;
    long days = total / 86400;
    long hours = (total % 86400) / 3600;
    long mins = (total % 3600) / 60;

    if (days > 0)
        snprintf(out, size, "%ldd %ldh %ldm", days, hours, mins);
    else if (hours > 0)
        snprintf(out, size, "%ldh %ldm", hours, mins);
    else
        snprintf(out, size, "%ldm", mins);
}

static void join_path(char *out, size_t size, const char *left, const char *right)
{
    if (!right || !right[0]) {
        snprintf(out, size, "-");
        return;
    }

    snprintf(out, size, "%s%s", left, right);
}

int main(void)
{
    int color = getenv("NO_COLOR") == NULL;

    char os[256] = "unknown";
    char cpu[256] = "unknown";
    char uptime[64];
    char host[256] = "localhost";

    probe("/etc/os-release", "PRETTY_NAME", os, sizeof(os));
    probe("/proc/cpuinfo", "model name", cpu, sizeof(cpu));
    format_uptime(uptime, sizeof(uptime));
    gethostname(host, sizeof(host));

    const char *user = getenv("USER");
    if (!user)
        user = "user";

    struct utsname uts;
    const char *kernel = uname(&uts) == 0 ? uts.release : "unknown";

    long total_kb = meminfo_kb("MemTotal");
    long avail_kb = meminfo_kb("MemAvailable");
    long used_kb = total_kb - avail_kb;

    char info[8][384];
    int count = 0;

    snprintf(info[count++], sizeof(info[0]), "%s%s@%s%s",
             color ? BOLD : "", user, host, color ? RESET : "");
    snprintf(info[count++], sizeof(info[0]), "-----------");

    if (color)
        snprintf(info[count++], sizeof(info[0]), LABEL "OS" RESET ":       %s", os);
    else
        snprintf(info[count++], sizeof(info[0]), "OS:       %s", os);

    if (color)
        snprintf(info[count++], sizeof(info[0]), LABEL "Kernel" RESET ":   %s", kernel);
    else
        snprintf(info[count++], sizeof(info[0]), "Kernel:   %s", kernel);

    if (color)
        snprintf(info[count++], sizeof(info[0]), LABEL "Uptime" RESET ":   %s", uptime);
    else
        snprintf(info[count++], sizeof(info[0]), "Uptime:   %s", uptime);

    char shell[256];
    join_path(shell, sizeof(shell), "", getenv("SHELL"));
    if (color)
        snprintf(info[count++], sizeof(info[0]), LABEL "Shell" RESET ":    %s", shell);
    else
        snprintf(info[count++], sizeof(info[0]), "Shell:    %s", shell);

    if (color)
        snprintf(info[count++], sizeof(info[0]), LABEL "CPU" RESET ":      %s", cpu);
    else
        snprintf(info[count++], sizeof(info[0]), "CPU:      %s", cpu);

    if (color)
        snprintf(info[count++], sizeof(info[0]), LABEL "Memory" RESET ":   %ldMiB / %ldMiB",
                 used_kb / 1024, total_kb / 1024);
    else
        snprintf(info[count++], sizeof(info[0]), "Memory:   %ldMiB / %ldMiB",
                 used_kb / 1024, total_kb / 1024);

    int logo_count = (int)(sizeof(logo) / sizeof(logo[0]));
    int rows = count > logo_count ? count : logo_count;

    for (int i = 0; i < rows; i++) {
        const char *left = i < logo_count ? logo[i] : "";
        const char *right = i < count ? info[i] : "";
        printf("%-30s  %s\n", left, right);
    }

    return 0;
}