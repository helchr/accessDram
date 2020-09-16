#ifndef CONFIG
#define CONFIG 

#include <string>
#include <vector>

class Config
{

public:
    std::vector<std::vector<std::vector<uint8_t>>> indexBits;
    void readFile(const std::string &path);
    static std::vector<uint8_t> parseCsv(const std::string &str);
    uint64_t indexFunction(uint64_t addr, const std::vector<std::vector<uint8_t>> &indexes);
    uint64_t indexFunctionChannel(uint64_t addr);
    uint64_t indexFunctionRank(uint64_t addr);
    uint64_t indexFunctionBank(uint64_t addr);
};

#endif