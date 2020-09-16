#include "config.h"
#include <sstream>
#include <fstream>

std::vector<uint8_t> Config::parseCsv(const std::string &str)
{
    std::vector<uint8_t> numbers;
    std::stringstream ss(str);
    std::string num;
    while (std::getline(ss, num, ','))
    {
        numbers.push_back(static_cast<uint8_t>(std::stoul(num)));
    }
    return numbers;
}

void Config::readFile(const std::string &path)
{
    std::ifstream ifs(path, std::ios::in);
    std::string buffer;
    indexBits.push_back(std::vector<std::vector<uint8_t>>());
    while (std::getline(ifs, buffer))
    {
        if (buffer == "")
        {
            indexBits.push_back(std::vector<std::vector<uint8_t>>());
            continue;
        }
        indexBits.back().push_back(std::vector<uint8_t>());
        auto numbers = parseCsv(buffer);
        indexBits.back().back() = numbers;
    }
}

uint64_t Config::indexFunction(uint64_t addr, const std::vector<std::vector<uint8_t>> &indexes)
{
    uint64_t result = 0;
    for (size_t i = 0; i < indexes.size(); i++)
    {
        auto bitUse = indexes[i];
        uint64_t outBit = 0;
        for (auto b : bitUse)
        {
            auto mask = 1ULL << b;
            outBit ^= (addr & mask) >> b;
        }
        result |= (outBit << i);
    }
    return result;
}

uint64_t Config::indexFunctionChannel(uint64_t addr)
{
    return indexFunction(addr, indexBits.at(0));
}

uint64_t Config::indexFunctionRank(uint64_t addr)
{
    return indexFunction(addr, indexBits.at(1));
}

uint64_t Config::indexFunctionBank(uint64_t addr)
{
    return indexFunction(addr, indexBits.at(2));
}

