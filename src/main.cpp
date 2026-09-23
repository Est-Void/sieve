#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

bool pump(std::istream& is, std::ostream& os) {
    std::string line;
    while (std::getline(is, line)) {
        os << line << '\n';
    }
    return !is.bad();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: sj <filter>" << '\n';
        return 1;
    }
    std::string_view filter = argv[1];
    if (filter != ".") {
        std::cerr << "Filter must be a single dot" << '\n';
        return 1;
    }
    
    if (argc == 2) {
        if (!pump(std::cin, std::cout)) {
            std::cerr << "Error reading from stdin" << '\n';
            return 1;
        }   
    }
    else {
        for (int i = 2; i < argc; ++i) {
            std::ifstream ifs(argv[i]);
            if (!ifs) {
                std::cerr << "Error opening file " << argv[i] << '\n';
                return 1;
            }
            if (!pump(ifs, std::cout)) {
                std::cerr << "Error reading file " << argv[i] << '\n';
                return 1;
            }
        }
    }
    
    return 0;
}
