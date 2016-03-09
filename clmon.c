/*
 * by nozino (nozino@gmail.com)
 */

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

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
  int cach;
  int free;
} mem_usage;

typedef struct _disk_usage {
  int read;
  int write;
} disk_usage;

typedef struct _disk_stat {
  char name[10];
  int read;
  int write;
} disk_stat;

typedef struct _net_stat {
  char name[10];
  int recieve;
  int transmit;
} net_stat;

#define DISK_STAT_MAX 10
#define NET_STAT_MAX 10

#define CPU_STAT "/proc/stat"
#define MEM_STAT "/proc/meminfo"
#define DISK_STAT "/proc/diskstats"
#define NET_STAT "/proc/net/dev"

static unsigned int cpu_stat[10];
static unsigned int mem_stat[10];
static disk_stat disk_stats[DISK_STAT_MAX];
static net_stat net_stats[NET_STAT_MAX];

#define CPU_USR 0
#define CPU_SYS 2
#define CPU_IDL 3
#define CPU_WAI 4
#define CPU_SIQ 6
#define CPU_HIQ 5

#define MEM_TOTAL 0
#define MEM_FREE 1
#define MEM_BUFF 2
#define MEM_CACH 3

static char *mem_stat_names[4] = {"MemTotal", "MemFree", "Buffers", "Cached"};

void print_usages() {
  printf("cmd [options] interval\n");
}

void _log(FILE *out, const char *fmt, va_list ap)
{
  vfprintf(out, fmt, ap);

}

void cl_log(char *log, ...) {
  va_list ap;

  va_start(ap, log);
  _log(stdout, log, ap);
  va_end(ap);
}

void cl_err(char *log, ...) {
  va_list ap;

  va_start(ap, log);
  _log(stderr, log, ap);
  va_end(ap);
}

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
    memcpy(str, tmp, strlen(tmp));
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

  tmp = strtok_r(buf, " ", &saveptr);
  for (i = 0; i <= 6; i++) {
    tmp = strtok_r(NULL, " ", &saveptr);
    stats[i] = atoi(tmp);
  }

  fclose(pf);

  return 0;
}

int get_mem_stat(unsigned int *stat) {
  FILE *pf;
  char buf[1024] = {0};

  char *tmp = NULL;
  char *saveptr = NULL;

  int i = 0;

  char key_of_line[512] = {0};
  char value[512];

  pf = fopen(MEM_STAT, "r");
  if (pf == NULL) {
    cl_log("cannot open file., path=%s", MEM_STAT);
  }

  while (!feof(pf)) {
    fgets(buf, 1024, pf);

    tmp = strtok_r(buf, ":", &saveptr);
    strcpy(key_of_line, tmp);

    remove_blanks(key_of_line);

    if (strcmp(key_of_line, "") == 0) {
      continue;
    }

    for (i = 0; i < 4; i++) {
      if (strcmp(key_of_line, mem_stat_names[i]) == 0) {
	tmp = strtok_r(saveptr, " ", &saveptr);
	strcpy(value, tmp);

	stat[i] = (unsigned int)atoi(value);
	break;
      }
    }
  }

  fclose(pf);

  return 0;
}

int get_disk_stat(unsigned int *stats) {
  FILE *pf;
  char buf[1024] = {0};

  char *tmp = NULL;
  char *saveptr = NULL;

  unsigned int i = 0;

  pf = fopen(DISK_STAT, "r");
  if (pf == NULL) {
    cl_err("cannot open file., path=%s", DISK_STAT);
  }

  while (fgets(buf, 1024, pf) > 0) {
    if (strlen(buf) <= 0) {
      fclose(pf);
      cl_err("cannot get stat.");
    }

    /*
      The /proc/diskstats file displays the I/O statistics
      of block devices. Each line contains the following 14
      fields:
        1  - major number
        2  - minor mumber
        3  - device name
        4  - reads completed successfully
        5  - reads merged
        6  - sectors read
        7  - time spent reading (ms)
        8  - writes completed
        9  - writes merged
        10 - sectors written
        11 - time spent writing (ms)
        12 - I/Os currently in progress
        13 - time spent doing I/Os (ms)
    */

    strtok_r(buf, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);

    tmp = strtok_r(NULL, " ", &saveptr);
    strcpy(disk_stats[i].name, tmp);

    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);

    tmp = strtok_r(NULL, " ", &saveptr);
    disk_stats[i].read = atoi(tmp);

    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);

    tmp = strtok_r(NULL, " ", &saveptr);
    disk_stats[i].write = atoi(tmp);

    if (i < DISK_STAT_MAX) {
      i++;
    } else {
      cl_err("disk count is bigger then the max count of buffer.");
      break;
    }
  }

  fclose(pf);

  return 0;
}

int get_net_stat(unsigned int *stats) {
  FILE *pf;
  char buf[1024] = {0};

  char *tmp = NULL;
  char *saveptr = NULL;

  unsigned int i = 0;

  pf = fopen(NET_STAT, "r");
  if (pf == NULL) {
    fprintf(stderr, "cannot open file., path=%s", NET_STAT);
  }


  fgets(buf, 1024, pf);
  fgets(buf, 1024, pf);
  while (fgets(buf, 1024, pf) > 0) {
    if (strlen(buf) <= 0) {
      fclose(pf);
      cl_err("cannot get stat.");
    }

    strtok_r(buf, " ", &saveptr);
    remove_blanks(buf);
    if (strcmp(buf, "lo:") == 0) {
      continue;
    }

    strcpy(net_stats[i].name, buf);

    tmp = strtok_r(NULL, " ", &saveptr);
    net_stats[i].recieve = atoi(tmp);

    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);
    strtok_r(NULL, " ", &saveptr);

    tmp = strtok_r(NULL, " ", &saveptr);
    net_stats[i].transmit = atoi(tmp);

    if (i < NET_STAT_MAX) {
      i++;
    } else {
      cl_err("network interface count is bigger then the max count of buffer.");
      break;
    }

  }

  fclose(pf);

  return 0;
}

/**
 * @cu
 * @interval seconds
 **/
/*
int get_cpu_usage(unsigned int cpu_stat[10], unsigned int now, cpu_usage *cu) {
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
*/
int get_mem_usage(mem_usage *mu, int interval) {
  if (mu == NULL) {
    err(10, "invalid args.");
  }

  sleep(interval);

  mu->used = 100;
  
  return 0;
}

int get_stats() {
  if (get_cpu_stat(cpu_stat) < 0) {
    err(2, "cannot get cpu stats.");
  }

  if (get_mem_stat(mem_stat) < 0) {
    err(2, "cannot get mem stats.");
  }

  if (get_disk_stat(NULL) < 0) {
    err(2, "cannot get disk stats.");
  }

  if (get_net_stat(NULL) < 0) {
    err(2, "cannot get network interface stats.");
  }

  return 0;
}

int init(int argc, char **argv) {
  if (argc == 2) {
    int interval = 0;
    interval = atoi(argv[1]);

    cl_log("interval = %d\n", interval);
    if (interval > 0) {
      //      g_interval = interval;
    } else {
      cl_err("invalid interval");
      print_usages();
      exit(-1);
    }
  }
  
  return 0;
}

void print_json() {
  unsigned int i = 0;

  printf("{");

  // cpu
  printf("\"cpu\":{");
  printf("\"usr\":%d,\"sys\":%d,\"idl\":%d,\"wai\":%d,\"siq\":%d,\"hiq\":%d", \
	 cpu_stat[CPU_USR],			      \
	 cpu_stat[CPU_SYS],			      \
	 cpu_stat[CPU_IDL],			      \
	 cpu_stat[CPU_WAI],			      \
	 cpu_stat[CPU_SIQ],			      \
	 cpu_stat[CPU_HIQ]			      \
	 );
  printf("}");
  // end of cpu

  // memory
  printf(",\"memory\":{");
  printf("\"total\":%d,\"free\":%d,\"buff\":%d,\"cach\":%d", \
	 mem_stat[MEM_TOTAL],			      \
	 mem_stat[MEM_FREE],			      \
	 mem_stat[MEM_BUFF],			      \
	 mem_stat[MEM_CACH]			      \
	 );
  printf("}");
  // end of memory

  // disk
  printf(",disk:[");

  while (strlen(disk_stats[i].name) > 0) {
    if (i > 0) printf(",");
    printf("{\"name\":\"%s\",\"read\":%d,\"write\":%d}",		      \
	   disk_stats[i].name,			      \
	   disk_stats[i].read,			      \
	   disk_stats[i].write			      \
	 );

    i++;
  }

  printf("]");
  // end of disk

  // net
  i = 0;
  printf(",net:[");

  while (strlen(net_stats[i].name) > 0) {
    if (i > 0) printf(",");
    printf("{\"name\":\"%s\",\"recieve\":%d,\"transmit\":%d}",		      \
	   net_stats[i].name,			      \
	   net_stats[i].recieve,			      \
	   net_stats[i].transmit			      \
	 );

    i++;
  }

  printf("]");
  // end of net

  printf("}");
}

int main(int argc, char **argv) {
  if (init(argc, argv) < 0) {
    print_usages();
  }

  get_stats();
  print_json();

  return 0;
}
