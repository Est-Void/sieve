#include "args.h"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace io {

inline bool write_all(int fd, const char *data, std::size_t n) noexcept {
  while (n > 0) {
    ssize_t w = ::write(fd, data, n);
    if (w < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    if (w == 0) {
      errno = EIO;
      return false;
    }
    data += w;
    n -= static_cast<std::size_t>(w);
  }
  return true;
}

inline bool copy_fd(int in_fd, int out_fd) {
  static thread_local std::vector<char> buf(256 * 1024);
  for (;;) {
    ssize_t r;
    do {
      r = ::read(in_fd, buf.data(), buf.size());
    } while (r < 0 && errno == EINTR);
    if (r < 0) {
      return false;
    }
    if (r == 0) {
      return true;
    }
    if (!write_all(out_fd, buf.data(), static_cast<std::size_t>(r))) {
      return false;
    }
  }
}

// Итог попытки копирования средствами ядра: Done — всё ушло,
// NotStarted — не скопировано ни байта (можно фолбэк на user-space),
// Failed — ошибка в середине, часть данных уже ушла в out_fd.
enum class PumpResult { Done, NotStarted, Failed };

// Файл → любой fd без прохода через user-space. Порция ограничена
// максимумом sendfile (0x7ffff000).
inline PumpResult try_sendfile(int in_fd, int out_fd, std::size_t size) {
  off_t off = 0;
  while (static_cast<std::size_t>(off) < size) {
    std::size_t chunk = size - static_cast<std::size_t>(off);
    if (chunk > 0x7ffff000u) {
      chunk = 0x7ffff000u;
    }
    ssize_t s = ::sendfile(out_fd, in_fd, &off, chunk);
    if (s < 0) {
      if (errno == EINTR) {
        continue;
      }
      return off == 0 ? PumpResult::NotStarted : PumpResult::Failed;
    }
    if (s == 0) {
      return off == 0 ? PumpResult::NotStarted : PumpResult::Failed;
    }
  }
  return PumpResult::Done;
}

// Всё, что ядро умеет сплайсить (pipe↔pipe, pipe↔файл) — тоже без
// user-space копии. Для несплайсимых fd (tty и т.п.) — NotStarted.
inline PumpResult try_splice(int in_fd, int out_fd) {
  for (;;) {
    ssize_t s = ::splice(in_fd, nullptr, out_fd, nullptr, 1 << 20,
                         SPLICE_F_MOVE);
    if (s < 0) {
      if (errno == EINTR) {
        continue;
      }
      return PumpResult::NotStarted;
    }
    if (s == 0) {
      return PumpResult::Done; // EOF
    }
  }
}

// stdin → stdout: сначала ядро, при невозможности — обычный read/write.
inline bool pump_stdin(int in_fd, int out_fd) {
  switch (try_splice(in_fd, out_fd)) {
  case PumpResult::Done:
    return true;
  case PumpResult::Failed:
    return false;
  case PumpResult::NotStarted:
    return copy_fd(in_fd, out_fd);
  }
  return false;
}

inline bool copy_file(const char *path, int out_fd) {
  int fd;
  do {
    fd = ::open(path, O_RDONLY | O_CLOEXEC);
  } while (fd < 0 && errno == EINTR);
  if (fd < 0) {
    return false;
  }

  struct stat st{};
  if (::fstat(fd, &st) != 0) {
    ::close(fd);
    return false;
  }

  if (!S_ISREG(st.st_mode)) {
    bool ok = copy_fd(fd, out_fd);
    ::close(fd);
    return ok;
  }

  if (st.st_size == 0) {
    ::close(fd);
    return true;
  }

  ::posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_WILLNEED);

  switch (try_sendfile(fd, out_fd, static_cast<std::size_t>(st.st_size))) {
  case PumpResult::Done:
    ::close(fd);
    return true;
  case PumpResult::Failed:
    ::close(fd);
    return false;
  case PumpResult::NotStarted:
    break; // sendfile недоступен (O_APPEND stdout, старое ядро...) — ниже
  }

  void *map = ::mmap(nullptr, static_cast<std::size_t>(st.st_size), PROT_READ,
                     MAP_PRIVATE, fd, 0);
  if (map == MAP_FAILED) {
    bool ok = copy_fd(fd, out_fd);
    ::close(fd);
    return ok;
  }

  ::madvise(map, static_cast<std::size_t>(st.st_size),
            MADV_SEQUENTIAL | MADV_WILLNEED);

  bool ok = write_all(out_fd, static_cast<const char *>(map),
                      static_cast<std::size_t>(st.st_size));
  ::munmap(map, static_cast<std::size_t>(st.st_size));
  ::close(fd);
  return ok;
}

} // namespace io

int main(int argc, char *argv[]) {
  Args args;
  std::string_view bad_token;
  switch (parse_args(argc, argv, args, bad_token)) {
  case ParseError::None:
    break;
  case ParseError::MissingFilter:
    std::cerr << "Missing <filter>\n";
    print_usage(std::cerr);
    return 2;
  case ParseError::BadFilter:
    std::cerr << "Unsupported filter (only \".\" is supported): " << bad_token
              << '\n';
    print_usage(std::cerr);
    return 2;
  case ParseError::UnknownFlag:
    std::cerr << "Unknown option: " << bad_token << '\n';
    print_usage(std::cerr);
    return 2;
  case ParseError::Conflict:
    std::cerr << "Conflicting options: -c/--compact and -p/--pretty\n";
    print_usage(std::cerr);
    return 2;
  }

  if (args.help) {
    print_usage(std::cout);
    return 0;
  }

  if (args.files.empty()) {
    if (!io::pump_stdin(STDIN_FILENO, STDOUT_FILENO)) {
      std::cerr << "Error copying stdin: " << std::strerror(errno) << '\n';
      return 1;
    }
    return 0;
  }

  for (const auto &path : args.files) {
    if (path == "-") {
      if (!io::pump_stdin(STDIN_FILENO, STDOUT_FILENO)) {
        std::cerr << "Error copying stdin: " << std::strerror(errno) << '\n';
        return 1;
      }
      continue;
    }
    if (!io::copy_file(path.c_str(), STDOUT_FILENO)) {
      std::cerr << "Error processing file " << path << ": "
                << std::strerror(errno) << '\n';
      return 1;
    }
  }

  return 0;
}
