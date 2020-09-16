#ifndef ADDRESS_TRANSLATION
#define ADDRESS_TRANSLATION

#include <cstdint>

class AddressTranslation 
{
    private:
        int pagemapFd;

    public:
        AddressTranslation();
        ~AddressTranslation();
        uint64_t frameNumberFromPagemap(uint64_t value);
        uint64_t getPhysicalAddr(uint64_t virtualAddr);

};

#endif