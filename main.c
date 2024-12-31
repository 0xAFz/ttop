#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include <stdio.h>

void get_memory_usage(double *total, double *used) {
  FILE *fp = fopen("/proc/meminfo", "r");
  if (!fp) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  char key[256];
  unsigned long value;
  unsigned long mem_total = 0, mem_free = 0, buffers = 0, cached = 0;

  while (fscanf(fp, "%s %lu kB", key, &value) == 2) {
    if (strcmp(key, "MemTotal:") == 0) {
      mem_total = value;
    } else if (strcmp(key, "MemFree:") == 0) {
      mem_free = value;
    } else if (strcmp(key, "Buffers:") == 0) {
      buffers = value;
    } else if (strcmp(key, "Cached:") == 0) {
      cached = value;
    }

    if (mem_total && mem_free && buffers && cached) {
      break;
    }
  }

  fclose(fp);

  *total = mem_total / 1024.0;                                // Convert to MB
  *used = (mem_total - mem_free - buffers - cached) / 1024.0; // Convert to MB
}

void get_cpu_times(unsigned long long *idle, unsigned long long *total) {
  FILE *fp = fopen("/proc/stat", "r");
  if (!fp) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  char buffer[256];
  if (fgets(buffer, sizeof(buffer), fp)) {
    unsigned long long user, nice, system, idle_time, iowait, irq, softirq,
        steal;
    sscanf(buffer, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice,
           &system, &idle_time, &iowait, &irq, &softirq, &steal);

    *idle = idle_time + iowait;
    *total = user + nice + system + idle_time + iowait + irq + softirq + steal;
  }

  fclose(fp);
}

void get_uptime(double *uptime, double *idle_time) {
  FILE *fp = fopen("/proc/uptime", "r");
  if (!fp) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  if (fscanf(fp, "%lf %lf", uptime, idle_time) != 2) {
    perror("fscanf");
    fclose(fp);
    exit(EXIT_FAILURE);
  }

  fclose(fp);
}

void get_os_info(char *os, size_t size) {
  FILE *fp = fopen("/etc/os-release", "r");
  if (!fp) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    if (strncmp(line, "NAME=", 5) == 0) {
      char *start = strchr(line, '=');
      if (start) {
        start++;
        char *end = strrchr(start, '\n');
        if (end)
          *end = '\0';

        if (start[0] == '"' || start[0] == '\'')
          start++;
        char *quote_end = strrchr(start, start[-1]);
        if (quote_end)
          *quote_end = '\0';

        strncpy(os, start, size - 1);
        os[size - 1] = '\0';
      }
      break;
    }
  }

  fclose(fp);
}

int main() {
  char os[128];
  unsigned long long idle1, total1, idle2, total2;
  double total_ram, used_ram, uptime, idle_time;
  int refresh_interval = 2; // Refresh every 2 seconds

  while (1) {
    get_cpu_times(&idle1, &total1);
    sleep(1);
    get_cpu_times(&idle2, &total2);

    unsigned long long idle_diff = idle2 - idle1;
    unsigned long long total_diff = total2 - total1;

    double cpu_usage = (1.0 - (double)idle_diff / total_diff) * 100.0;

    get_memory_usage(&total_ram, &used_ram);

    get_uptime(&uptime, &idle_time);
    int hours = (int)uptime / 3600;
    int minutes = ((int)uptime % 3600) / 60;
    int seconds = (int)uptime % 60;

    get_os_info(os, sizeof(os));

    system("clear");

    printf("CPU: %.2f%%\tRAM: %.2fMB/%.2f\t Uptime: %dh, %dm, %ds\tOS: %s\n",
           cpu_usage, used_ram, total_ram, hours, minutes, seconds, os);

    sleep(refresh_interval);
  }

  return 0;
}
