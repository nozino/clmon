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

int get_cpu_stat(cpu_usage *cu) {
  FILE *pf;
  char buf[1024] = {0};

  char *tmp = NULL;
  char *saveptr = NULL;

  int i = 0;
  unsigned int stats[6] = {0};

  if (cu == NULL) {
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

  printf("HERE, buf=%s\n", buf);
  tmp = strtok_r(buf, " ", &saveptr);
  for (i = 0; i < 6; i++) {
    tmp = strtok_r(NULL, " ", &saveptr);
    printf("tmp=%s\n", tmp);
    stats[i] = atoi(tmp);
    printf("stats[%d]=%d\n", i, stats[i]);
  }

  cu->usr = stats[0];
  cu->sys = stats[2];
  cu->idl = stats[3];
  cu->wai = stats[4];
  cu->hiq = stats[5];
  cu->siq = stats[6];

  fclose(pf);

  return 1;
}

/**
 * @cu
 * @interval second
 **/
int get_cpu_usage(cpu_usage *cu, const unsigned int interval) {
  cpu_usage cu_a;
  cpu_usage cu_b;

  unsigned int total_a = 0;
  unsigned int total_b = 0;
  unsigned int total = 0;


  if (cu == NULL) {
    err(10, "invalid args.");
  }

  if (get_cpu_stat(&cu_a) < 0) {
    err(2, "cannot get cpu stat.");
  }

  sleep(interval);

  if (get_cpu_stat(&cu_b) < 0) {
    err(2, "cannot get cpu stat.");
  }

  cu->usr = cu_b.usr - cu_a.usr;
  cu->sys = cu_b.sys - cu_a.sys;
  cu->wai = cu_b.wai - cu_a.wai;
  cu->idl = cu_b.idl - cu_a.idl;
  cu->siq = cu_b.siq - cu_a.siq;
  cu->hiq = cu_b.hiq - cu_a.hiq;

  total_a = cu_a.usr + cu_a.sys + cu_a.wai + cu_a.hiq + cu_a.siq;
  total_b = cu_b.usr + cu_b.sys + cu_b.wai + cu_b.hiq + cu_b.siq;

  total = total_b - total_a;

  printf("total=%d, total_a=%d, total_b=%d\n", total, total_a, total_b);

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

int main(int argc, char **argv) {
  char value[1024] = {0};
  int retval = 0;
  char path[100] = "/proc/cpuinfo";
  char key[100] = "model name";

  cpu_usage *cu;
  mem_usage *mu;

  retval = get_value(path, "model name", ":", value);
  if (retval < 0) {
    fprintf(stderr, "cannot get the value, key=%s", key);
    return -2;
  } else if (retval == 1) {
    printf("cannot find the value. key=%s\n", key);
    return -3;
  }

  printf("path=%s, key=%s, value=%s\n", path, key, value);

  cu = (cpu_usage *)malloc(sizeof(cpu_usage));
  if (cu == NULL) {
    err(10, "cannot alloc for cpu usage.");
  }

  if (get_cpu_usage(cu, 1) != 0) {
    err(10, "cannot get cpu usage.");
  }

  printf("cpu_usage.usr=%.2f, sys=%.2f, wai=%.2f, idl=%.2f.\n", cu->usr, cu->sys, cu->wai, cu->idl);
  
  mu = (mem_usage *)malloc(sizeof(mem_usage));
  if (mu == NULL) {
    err(10, "cannot alloc for memory usage.");
  }

  if (get_mem_usage(mu, 1) != 0) {
    err(10, "cannot get memory usage.");
  }

  printf("mem_usage.used=%d.\n", mu->used);
  return 0;
}
