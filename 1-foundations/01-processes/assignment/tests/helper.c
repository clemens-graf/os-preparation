/*
 * helper.c - small test program that minish runs during the tests.
 *
 *   helper exit N     exit with status N
 *   helper signal N   kill itself with signal N
 *   helper zombies    print how many zombie children the parent (minish) has
 *   helper pipefds    print how many pipe fds the parent (minish) holds open
 */
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int count_zombie_siblings(void)
{
  pid_t parent = getppid();
  DIR *proc = opendir("/proc");
  if (!proc)
    return -1;
  int zombies = 0;
  struct dirent *e;
  while ((e = readdir(proc))) {
    char *end;
    long pid = strtol(e->d_name, &end, 10);
    if (*end || pid <= 0)
      continue;
    char path[64], buf[512];
    snprintf(path, sizeof path, "/proc/%ld/stat", pid);
    FILE *f = fopen(path, "r");
    if (!f)
      continue;
    if (fgets(buf, sizeof buf, f)) {
      char *p = strrchr(buf, ')');
      char state;
      int ppid;
      if (p && sscanf(p + 2, "%c %d", &state, &ppid) == 2 && ppid == parent && state == 'Z')
        zombies++;
    }
    fclose(f);
  }
  closedir(proc);
  return zombies;
}

static int count_parent_pipes(void)
{
  char dir[64];
  snprintf(dir, sizeof dir, "/proc/%d/fd", getppid());
  DIR *d = opendir(dir);
  if (!d)
    return -1;
  int pipes = 0;
  struct dirent *e;
  while ((e = readdir(d))) {
    if (e->d_name[0] == '.')
      continue;
    char link[320], target[256];
    snprintf(link, sizeof link, "%s/%s", dir, e->d_name);
    ssize_t n = readlink(link, target, sizeof target - 1);
    if (n > 0) {
      target[n] = '\0';
      int fd = atoi(e->d_name);
      if (fd > 2 && strncmp(target, "pipe:", 5) == 0)
        pipes++;
    }
  }
  closedir(d);
  return pipes;
}

int main(int argc, char **argv)
{
  if (argc >= 3 && strcmp(argv[1], "exit") == 0)
    return atoi(argv[2]);
  if (argc >= 3 && strcmp(argv[1], "signal") == 0) {
    int sig = atoi(argv[2]);
    signal(sig, SIG_DFL);
    raise(sig);
    return 99;
  }
  if (argc >= 2 && strcmp(argv[1], "zombies") == 0) {
    printf("zombies: %d\n", count_zombie_siblings());
    return 0;
  }
  if (argc >= 2 && strcmp(argv[1], "pipefds") == 0) {
    printf("pipe fds: %d\n", count_parent_pipes());
    return 0;
  }
  fprintf(stderr, "usage: helper exit N | signal N | zombies | pipefds\n");
  return 2;
}
