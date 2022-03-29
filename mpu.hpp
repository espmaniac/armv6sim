#include <cstdint>
#include <math.h>

class MPU {
public:
    MPU();
    ~MPU();
    struct MemoryRegion {
        uint32_t base_address;
        uint32_t size;
        uint32_t access;
        uint32_t enable:1;
        uint8_t *memory;
    };
    
    MemoryRegion* getRegion(uint32_t);

    void createRegion(uint32_t, uint32_t);
    
    uint64_t regionSize(uint8_t) const;

    void write8(uint32_t, uint8_t);

    uint8_t read8(uint32_t);

    void write16(uint32_t, uint16_t);

    uint16_t read16(uint32_t);

    void write32(uint32_t, uint32_t);

    uint32_t read32(uint32_t);

    bool faultStatus() const;
    
private:
    MemoryRegion memreg[8];
    uint16_t regionCount;
    bool fault;
};

