#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "%s 0xMemoryLocation [/dev/mem]\n", argv[0]);
    return 2;
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

  int fd = open(file, O_RDONLY | O_SYNC);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  int last_errno = 0;
  for (;;) {
    size_t map_start = target & ~MAP_MASK;
    char *map = mmap(0, MAP_SIZE, PROT_READ, MAP_SHARED, fd, map_start);
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
    write(1, map + (target - map_start), MAP_SIZE - (target - map_start));
    if (map != empty) {
      munmap(map, MAP_SIZE);
    }
    target = map_start + MAP_SIZE;
  }
}
