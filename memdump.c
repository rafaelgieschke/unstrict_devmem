#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 4) {
    fprintf(stderr, "%s [-r] 0xMemoryLocation [/dev/mem]\n", argv[0]);
    return 2;
  }

  int revert = 0;
  if (!strcmp(argv[1], "-r")) {
    revert = 1;
    argv += 1;
  }

  size_t target = strtoll(argv[1], 0, 0);

  const char *file = "/dev/mem";
  if (argc > 2) {
    file = argv[2];
  }

  int on_error_resume_next = getenv("force") && *getenv("force");

  fprintf(stderr, "%s @ 0x%lx:\n", file, target);

  size_t MAP_SIZE = sysconf(_SC_PAGE_SIZE);
  unsigned long MAP_MASK = (MAP_SIZE - 1);
  char empty[MAP_SIZE];
  memset(empty, 0, sizeof empty);

  int fd = open(file, (revert ? O_RDWR : O_RDONLY) | O_SYNC);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  int last_errno = 0;
  for (;;) {
    size_t map_start = target & ~MAP_MASK;
    char *map = mmap(0, MAP_SIZE, revert ? PROT_WRITE : PROT_READ, MAP_SHARED, fd, map_start);
    if (map == MAP_FAILED) {
      if (errno != last_errno) {
        last_errno = errno;
        fprintf(stderr, "mmap@0x%08lx: %s\n", map_start, strerror(last_errno));
      }
      if (!on_error_resume_next) {
        return 1;
      }
      map = empty;
    } else {
      if (last_errno) {
        last_errno = 0;
        fprintf(stderr, "mmap@0x%08lx: %s\n", map_start, strerror(last_errno));
      }
    }
    if (revert) {
      size_t bytes = read(0, map + (target - map_start), MAP_SIZE - (target - map_start));
      if (bytes == -1) {
        perror("read");
        return 1;
      }
      if (bytes == 0) {
        return 0;
      }
      target += bytes;
      if (map == empty) {
        memset(empty, 0, sizeof empty);
      }
    } else {
      size_t bytes = write(1, map + (target - map_start), MAP_SIZE - (target - map_start));
      if (bytes == -1) {
        perror("write");
        return 1;
      }
      target = map_start + MAP_SIZE;
    }
    if (map != empty) {
      munmap(map, MAP_SIZE);
    }
  }
}
