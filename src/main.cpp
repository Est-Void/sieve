#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

enum class Mode {
    Compact,
    Pretty
};

struct Args {
    std::string_view filter;
    Mode mode = Mode::Compact;
    std::vector<std::string> files;
    bool help = false;
};

enum class ParseError {
    None,
    MissingFilter,
    BadFilter,
    UnknownFlag,
    Conflict
};


void print_usage(std::ostream& os) {
    os << "Usage: sj [-c|--compact|-p|--pretty] [-h|--help] <filter> [file...]\n";
}

ParseError parse_args(int argc, char* argv[], Args& args, std::string_view& bad_token){
    bool only_files = false;
    bool seen_c = false;
    bool seen_p = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (!only_files) {
            if (arg == "-h" || arg == "--help") {
                args.help = true;
                continue;
            }
            else if (arg == "-c" || arg == "--compact") {
                args.mode = Mode::Compact;
                seen_c = true;
                continue;
            }
            else if (arg == "-p" || arg == "--pretty") {
                args.mode = Mode::Pretty;
                seen_p = true;
                continue;
            }
            else if (arg == "--") {
                only_files = true;
                continue;
            }
            else if (arg.size() > 1 && arg[0] == '-') {
                bad_token = arg;
                return ParseError::UnknownFlag;
            }
        }
        if (args.filter.empty()) {
            args.filter = arg;
        } else {
            args.files.emplace_back(arg);
        }
    }
    if (args.help) {
        return ParseError::None;
    }
    if (args.filter.empty()) {
        bad_token = {};
        return ParseError::MissingFilter;
    }
    if (seen_c && seen_p) {
        bad_token = {};
        return ParseError::Conflict;
    }
    if (args.filter != ".") {
        bad_token = args.filter;
        return ParseError::BadFilter;
    }
    return ParseError::None;
}

namespace io {
inline bool write_all(int fd, const char* data, std::size_t n) noexcept {
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

inline bool copy_file(const char* path, int out_fd) {
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

    void* map = ::mmap(nullptr, static_cast<std::size_t>(st.st_size),
                       PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (map == MAP_FAILED) {
        int fd2;
        do {
            fd2 = ::open(path, O_RDONLY | O_CLOEXEC);
        } while (fd2 < 0 && errno == EINTR);
        if (fd2 < 0) {
            return false;
        }
        bool ok = copy_fd(fd2, out_fd);
        ::close(fd2);
        return ok;
    }

    ::madvise(map, static_cast<std::size_t>(st.st_size),
              MADV_SEQUENTIAL | MADV_WILLNEED);

    bool ok = write_all(out_fd, static_cast<const char*>(map),
                        static_cast<std::size_t>(st.st_size));
    ::munmap(map, static_cast<std::size_t>(st.st_size));
    return ok;
}

} 

int main(int argc, char* argv[]) {
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
            std::cerr << "Unsupported filter (only \".\" is supported): " << bad_token << '\n';
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
        if (!io::copy_fd(STDIN_FILENO, STDOUT_FILENO)) {
            std::cerr << "Error copying stdin: " << std::strerror(errno) << '\n';
            return 1;
        }
        return 0;
    }

    for (const auto& path : args.files) {
        if (path == "-") {
            if (!io::copy_fd(STDIN_FILENO, STDOUT_FILENO)) {
                std::cerr << "Error copying stdin: " << std::strerror(errno) << '\n';
                return 1;
            }
            continue;
        }
        if (!io::copy_file(path.c_str(), STDOUT_FILENO)) {
            std::cerr << "Error processing file " << path
                      << ": " << std::strerror(errno) << '\n';
            return 1;
        }
    }

    return 0;
}
