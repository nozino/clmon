#include <stdio.h>
#include <string.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct _cpu_usage {
  float usr;
  float sys;
  float idl;
  float wai;
  float hiq;
  float siq;
} cpu_usage;

/**
 * MB
 **/
typedef struct _mem_usage {
  int used;
  int buff;
  int cache;
  int free;
} mem_usage;

static unsigned int cpu_stats[2][10] = {0};
static unsigned int mem_stats[2][10] = {0};


#define CPU_USR 0
#define CPU_SYS 2
#define CPU_IDL 3
#define CPU_WAI 4
#define CPU_SIQ 6
#define CPU_HIQ 5

int remove_blanks(char *str) {
  char *tmp = str;
  if (str == NULL) {
    perror("invalid args");
    return -1;
  }

  while (*tmp == ' ' || *tmp == '\t') {
    ++tmp;
  }

  if (str != tmp) {
    memcpy(str, tmp, sizeof(tmp));
  }

  tmp = &(str[strlen(str) - 1]);
  while (*tmp == ' ' || *tmp == '\t') {
    *tmp = 0;
    --tmp;
  }

  return 0;
}

int get_value(const char *path, const char *key, const char *delim, char *value) {
  FILE *p_file;
  char buf[1024] = {0};
  //  const unsigned int key_size = sizeof(key);
  char key_of_line[512] = {0};

  char *tmp = NULL;
  char *saveptr = NULL;

  if (path == NULL || key == NULL || value == NULL) {
    perror("invalid args.");
    return -1;
  }

  p_file = fopen(path, "r");

  if (p_file == NULL) {
    fprintf(stderr, "cannot open file., path=%s", path);
  }

  while (!feof(p_file)) {
    fgets(buf, 1024, p_file);

    tmp = strtok_r(buf, delim, &saveptr);
    strcpy(key_of_line, tmp);

    remove_blanks(key_of_line);

    if (strcmp(key_of_line, "") == 0) {
      continue;
    }
    
    if (strcmp(key, key_of_line) == 0) {
      strcpy(value, saveptr);
      remove_blanks(value);
      fclose(p_file);
      return 0;
    }
  }

  fclose(p_file);

  return 1;
}

#define CPU_STAT "/proc/stat"

int get_cpu_stat(unsigned int *stats) {
  FILE *pf;
  char buf[1024] = {0};

  char *tmp = NULL;
  char *saveptr = NULL;

  int i = 0;

  if (stats == NULL) {
    perror("invalid args.");
    return -1;
  }

  pf = fopen(CPU_STAT, "r");
  if (pf == NULL) {
    fprintf(stderr, "cannot open file., path=%s", CPU_STAT);
  }

  fgets(buf, 1024, pf);
  if (strlen(buf) <= 0) {
    fclose(pf);
    err(2, "cannot get stat.");
  }

  //  printf("buf=%s\n", buf);

  tmp = strtok_r(buf, " ", &saveptr);
  for (i = 0; i <= 6; i++) {
    tmp = strtok_r(NULL, " ", &saveptr);
    stats[i] = atoi(tmp);
  }

  fclose(pf);

  return 0;
}

void print_stat(unsigned int b[][10]){
  int i, j;

  for (i = 0; i < 2; i++) {
    for (j = 0;j < 10; j++) {
      printf("b[%d][%d]=%d\n", i, j, b[i][j]);
    }
  }
  
}

/**
 * @cu
 * @interval second
 **/
int get_cpu_usage(unsigned int cpu_stats[][10], unsigned int now, cpu_usage *cu) {
  cpu_usage cu_a;
  cpu_usage cu_b;

  unsigned int total_a = 0;
  unsigned int total_b = 0;
  unsigned int total = 0;

  unsigned int before = 0;

  if (cu == NULL || now < 0 || now > 1 || cpu_stats == NULL) {
    err(10, "invalid args. now=%d", now);
  }

  before = now == 1 ? 0 : 1;


  total_a = cpu_stats[before][0] + cpu_stats[before][1] + cpu_stats[before][2] + cpu_stats[before][3] + cpu_stats[before][4] + cpu_stats[before][5] + cpu_stats[before][6];
  total_b = cpu_stats[now][0] + cpu_stats[now][1] + cpu_stats[now][2] + cpu_stats[now][3] + cpu_stats[now][4] + cpu_stats[now][5] + cpu_stats[now][6];

  total = total_b - total_a;

  cu->usr = (float)(((float)(cpu_stats[now][CPU_USR] - cpu_stats[before][CPU_USR]) / total) * 100.0);
  cu->sys = (float)(((float)(cpu_stats[now][CPU_SYS] - cpu_stats[before][CPU_SYS]) / total) * 100.0);
  cu->wai = (float)(((float)(cpu_stats[now][CPU_WAI] - cpu_stats[before][CPU_WAI]) / total) * 100.0);
  cu->idl = (float)(((float)(cpu_stats[now][CPU_IDL] - cpu_stats[before][CPU_IDL]) / total) * 100.0);
  cu->siq = (float)(((float)(cpu_stats[now][CPU_SIQ] - cpu_stats[before][CPU_SIQ]) / total) * 100.0);
  cu->hiq = (float)(((float)(cpu_stats[now][CPU_HIQ] - cpu_stats[before][CPU_HIQ]) / total) * 100.0);

  return 0;
}

int get_mem_usage(mem_usage *mu, int interval) {
  if (mu == NULL) {
    err(10, "invalid args.");
  }

  sleep(interval);

  mu->used = 100;
  
  return 0;
}

int get_stats(const unsigned int now) {
  if (now < 0 || now > 1) {
    err(1, "invalid args. now=%d", now);
  }
  
  if (get_cpu_stat(&cpu_stats[now][0]) < 0) {
    err(2, "cannot get cpu stats.");
  }
}

int main(int argc, char **argv) {
  char value[1024] = {0};
  int retval = 0;
  char path[100] = "/proc/cpuinfo";
  char key[100] = "model name";

  cpu_usage cu;
  mem_usage mu;

  unsigned int now = 0;
  unsigned int interval = 1; /* 1 second */

  retval = get_value(path, "model name", ":", value);
  if (retval < 0) {
    fprintf(stderr, "cannot get the value, key=%s", key);
    return -2;
  } else if (retval == 1) {
    printf("cannot find the value. key=%s\n", key);
    return -3;
  }

  printf("path=%s, key=%s, value=%s\n", path, key, value);

  for (;;) {

    if (get_stats(now) < 0) {
      err(1, "cannot get stats.");
    }

    //    print_stat(cpu_stats);

    get_cpu_usage(cpu_stats, now, &cu);

    printf("cpu_usage.usr=%.2f, sys=%.2f, wai=%.2f, idl=%.2f, siq=%.2f, hiq=%.2f.\n", cu.usr, cu.sys, cu.wai, cu.idl, cu.siq, cu.hiq);

    sleep(interval);

    now = now == 1 ? 0 : 1;
  }

  
  printf("mem_usage.used=%d.\n", mu.used);

  return 0;
}
