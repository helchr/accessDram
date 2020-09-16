#include <iostream>
#include <unistd.h>
#include <vector>
#include <sys/mman.h>
#include <x86intrin.h>
#include <algorithm>
#include <chrono>
#include <omp.h>
#include "addressTranslation.h"
#include "config.h"

Config config;
AddressTranslation addressTranslation;

struct clSize
{
    uint64_t a[8] = {0, 0, 0, 0, 0, 0, 0, 0};
} __attribute__((aligned(64)));

template <class T>
std::vector<size_t> getElements(T* array, size_t size, const std::vector<uint8_t>& channels,
                                const std::vector<uint8_t>& ranks, const std::vector<uint8_t>& banks)
{
#pragma omp declare reduction (merge : std::vector<size_t> : omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end()))

    std::vector<size_t> indexes;
    indexes.reserve(size);
    bool channelOk = true;
    bool bankOk = true;
    bool rankOk = true;
    #pragma omp parallel for reduction(merge: indexes)
    for(size_t i = 0; i < size; i++)
    {
        uint64_t physAddr = addressTranslation.getPhysicalAddr((uint64_t)&(array[i]));
        if(!channels.empty())
        {
            channelOk = false;
            auto ch = config.indexFunctionChannel(physAddr);
            if(std::find(std::begin(channels),std::end(channels),ch) != channels.end())
            {
                channelOk = true;
            }
        }
        if(!ranks.empty())
        {
            rankOk = false;
            auto ra = config.indexFunctionRank(physAddr);
            if(std::find(std::begin(ranks),std::end(ranks),ra) != ranks.end())
            {
                rankOk = true;
            }
        }
        if(!banks.empty())
        {
            bankOk = false;
            auto ba = config.indexFunctionBank(physAddr);
            if(std::find(std::begin(banks),std::end(banks),ba) != banks.end())
            {
                bankOk = true;
            }
        }
        if(channelOk && rankOk && bankOk)
        {
            indexes.push_back(i);
        }
    }
    return indexes;
}


template <class T>
std::vector<std::vector<size_t>> getElementsPartition(T *array, size_t size, const std::vector<uint8_t> &channels,
                                                      const std::vector<uint8_t> &ranks)
{
    #pragma omp declare reduction(merge : std::vector <size_t> : omp_out.insert(omp_out.end(), omp_in.begin(), omp_in.end()))
    std::vector<std::vector<size_t>> indexesThread;
    indexesThread.reserve(omp_get_max_threads());
    bool channelOk = true;
    bool bankOk = true;
    bool rankOk = true;
    for (int tid = 0; tid < omp_get_max_threads(); tid++)
    {
        indexesThread.push_back(std::vector<size_t>());
        auto &indexes = indexesThread[tid];
        #pragma omp parallel for reduction(merge : indexes)
        for (size_t i = 0; i < size; i++)
        {
            uint64_t physAddr = addressTranslation.getPhysicalAddr((uint64_t) & (array[i]));
            if (!channels.empty())
            {
                channelOk = false;
                auto ch = config.indexFunctionChannel(physAddr);
                if (std::find(std::begin(channels), std::end(channels), ch) != channels.end())
                {
                    channelOk = true;
                }
            }
            if (!ranks.empty())
            {
                rankOk = false;
                auto ra = config.indexFunctionRank(physAddr);
                if (std::find(std::begin(ranks), std::end(ranks), ra) != ranks.end())
                {
                    rankOk = true;
                }
            }
            bankOk = false;
            auto ba = config.indexFunctionBank(physAddr);
            if (ba == static_cast<uint64_t>(tid))
            {
                bankOk = true;
            }
            if (channelOk && rankOk && bankOk)
            {
                indexes.push_back(i);
            }
        }
    }
    return indexesThread;
}

template <class T>
void doAccessPartition(T *array, const std::vector<std::vector<size_t>> &indexesThread, size_t num)
{
    size_t idx;
    volatile T *arrayV = array;
    const auto rep = num / (indexesThread[0].size() * indexesThread.size());
    #pragma omp parallel for private(idx) schedule(static)
    for (size_t t = 0; t < indexesThread.size(); t++)
    {
        auto indexes = indexesThread.at(t);
        for (size_t i = 0; i < indexes.size(); i++)
        {
            for (size_t r = 0; r < rep; r++)
            {
                idx = indexes[i];
                arrayV[idx].a[0];
                _mm_clflush(&(array[idx]));
            }
        }
    }
}

template <class T>
void doAccess(T *array, const std::vector<size_t> &indexes, size_t num)
{
    size_t idx;
    volatile T *arrayV = array;
    const auto rep = num / (indexes.size());
    #pragma omp parallel for private(idx) schedule(static)
    for (size_t i = 0; i < indexes.size(); i++)
    {
        for (size_t r = 0; r < rep; r++)
        {
            idx = indexes[i];
            arrayV[idx].a[0];
            _mm_clflush(&(array[idx]));
        }
    }
}

clSize* allocateArray(size_t size)
{
    auto array = new clSize[size];
    #pragma omp parallel for
    for (size_t i = 0; i < size; i++)
    {
        array[i].a[0] = 0;
    }
    return array;
}


int main(int argc, char *argv[])
{
    int opt;
    std::string configFilePath = "";
    std::string channelStr = "";
    std::string rankStr = "";
    std::string bankStr = "";
    bool partition = false;
    int threads = 0;
    size_t size = 10 * 1024 * 1024ULL / sizeof(clSize);
    size_t numAccess = 800 * 1000 * 1000ULL;
    while ((opt = getopt(argc, argv, "f:t:s:n:c:r:b:p")) != -1)
    {
        switch (opt)
        {
        case 'f':
            configFilePath = optarg;
            break;
        case 's':
            size = static_cast<size_t>(atoi(optarg));
            break;
        case 'n':
            numAccess = static_cast<size_t>(atoi(optarg));
            break;
        case 'c':
            channelStr = optarg;
            break;
        case 'r':
            rankStr = optarg;
            break;
        case 'b':
            bankStr = optarg;
            break;
        case 'p':
            partition = true;
            break;
        case 't':
            threads = atoi(optarg);
            break;
        default:
            std::cout << "Usage: " << argv[0] << "-f <config file path> [-c <channel> | [-r <rank>] | [-c <bank>] | -s <array size> | -t <number of threads> | -p ]\n"
                      << "Must be run as root to resolve physical addresses\n";
            exit(EXIT_FAILURE);
        }
    }

    if (configFilePath == "")
    {
        std::cout << "No config file specified" << std::endl;
        return EXIT_FAILURE;
    }
    if(partition == true && bankStr != "")
    {
        std::cout << "Bank limitation together with partitioned access is not supported" << std::endl;
        return EXIT_FAILURE;
    }
    
    
    

    omp_set_num_threads(threads);
    config.readFile(configFilePath);
    auto channels = Config::parseCsv(channelStr);
    auto ranks = Config::parseCsv(rankStr);
    auto banks = Config::parseCsv(bankStr);

    auto array = allocateArray(size);
    std::cout << "Array allocated at: " << std::hex << addressTranslation.getPhysicalAddr((uint64_t)array) << std::endl;

    if(partition)
    {
        auto indexes = getElementsPartition(array, size, channels, ranks);
        for (int i = 0; i < threads; i++)
        {
            std::cout << "Number of Indexes for thread " << std::dec << i << ": " << indexes.at(i).size() << std::endl;
        }
        std::cout << "Number of accesses: " << std::dec << numAccess << std::endl;
        std::cout << "Access Type: Partitioned" << std::endl;
        auto ts = std::chrono::steady_clock::now();
        doAccessPartition(array, indexes, numAccess);
        auto te = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(te - ts);
        std::cout << "Time: " << diff.count() << "ms" << std::endl;
    }
    else
    {
        auto indexes = getElements(array,size,channels,ranks,banks);
        std::cout << "Number of accesses: " << std::dec << numAccess << std::endl;
        std::cout << "Access Type: Mixed" << std::endl;
        auto ts = std::chrono::steady_clock::now();
        doAccess(array, indexes, numAccess);
        auto te = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(te - ts);
        std::cout << "Time: " << diff.count() << "ms" << std::endl;
    }
    return 0;
}
