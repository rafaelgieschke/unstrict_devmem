#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "%s 0xMemoryLocation\n", argv[0]);
    return 2;
  }

  size_t target = strtoll(argv[1], 0, 0);

  const char file[] = "/dev/mem";
  fprintf(stderr, "%s @ 0x%lx:\n", file, target);

  size_t MAP_SIZE = sysconf(_SC_PAGE_SIZE);
  unsigned long MAP_MASK = (MAP_SIZE - 1);

  int fd = open(file, O_RDONLY | O_SYNC);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  for (;;) {
    size_t map_start = target & ~MAP_MASK;
    char *map = mmap(0, MAP_SIZE, PROT_READ, MAP_SHARED, fd, map_start);
    if (map == MAP_FAILED) {
      perror("mmap");
      return 1;
    }
    write(1, map + (target - map_start), MAP_SIZE - (target - map_start));
    munmap(map, MAP_SIZE);
    target = map_start + MAP_SIZE;
  }
}
