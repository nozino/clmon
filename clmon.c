#include <stdio.h>
#include <string.h>

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
}

int get_value(const char *path, const char *key, const char *delim, char *value) {
  FILE *p_file;
  char buf[1024] = {0};
  const unsigned int key_size = sizeof(key);
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

int main(int argc, char **argv) {
  char value[1024] = {0};
  int retval = 0;
  char path[100] = "/proc/cpuinfo";
  char key[100] = "model name";

  retval = get_value(path, "model name", ":", value);
  if (retval < 0) {
    fprintf(stderr, "cannot get value, key=%s", key);
    return -2;
  } else if (retval == 1) {
    printf("cannot find value. key=%s\n", key);
    return -3;
  }

  printf("path=%s, key=%s, value=%s\n", path, key, value);

  return 0;
}
