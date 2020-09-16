#include "addressTranslation.h"
#include <assert.h>
#include <fcntl.h>
#include <linux/kernel-page-flags.h>
#include <unistd.h>
#include <stdexcept>

AddressTranslation::AddressTranslation()
{
    pagemapFd = open("/proc/self/pagemap", O_RDONLY);
    if(pagemapFd < 0)
    {
        throw(std::runtime_error("Can't access pagemap"));
    }
}

AddressTranslation::~AddressTranslation()
{
    close(pagemapFd);
}

uint64_t AddressTranslation::frameNumberFromPagemap(uint64_t value)
{
    return value & ((1ULL << 54) - 1);
}

uint64_t AddressTranslation::getPhysicalAddr(uint64_t virtualAddr)
{
    uint64_t value;
    off_t offset = (virtualAddr / 4096) * sizeof(value);
    auto got = pread(pagemapFd, &value, sizeof(value), offset);
    assert(got == 8);
    // Check the "page present" flag.
    assert(value & (1ULL << 63));
    uint64_t frame_num = frameNumberFromPagemap(value);
    return (frame_num * 4096) | (virtualAddr & (4095));
}