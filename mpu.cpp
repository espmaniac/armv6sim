#include "mpu.hpp"

MPU::MPU() : regionCount(0) {
    //memreg = new MemoryRegion[8];
}

MPU::~MPU() {
    for (; regionCount > 0; --regionCount)
        delete[] memreg[regionCount - 1].memory;
}

MPU::MemoryRegion* MPU::getRegion(uint32_t addr) {
    MemoryRegion* region = nullptr;
    for (uint8_t i = 0; i < 8; ++i) {
        if (
            ((addr >= memreg[i].base_address) && (addr < (memreg[i].base_address + memreg[i].size)))
            &&
            memreg[i].enable
        )
            region = &memreg[i];
    }
    return region;
}

void MPU::createRegion(uint32_t address, uint32_t size) {
    memreg[regionCount].memory = new uint8_t[size]{0};
    memreg[regionCount].size = size;
    memreg[regionCount].base_address = address;
    memreg[regionCount].enable = 1;
    ++regionCount;
}

uint64_t MPU::regionSize(uint8_t registerSize) const {
    return pow(2, ((registerSize & 0x1F) + 1));
}

void MPU::write8(uint32_t addr, uint8_t value) {
    MemoryRegion *region = getRegion(addr);
    region->memory[addr - region->base_address] = value;
}

uint8_t MPU::read8(uint32_t addr) {
    MemoryRegion *region = getRegion(addr);
    return region->memory[addr - region->base_address];
}

void MPU::write16(uint32_t addr, uint16_t value) {
    MemoryRegion *region = getRegion(addr);
    
    //*(uint16_t*)&region->memory[addr - region->base_address] = value;
    addr -= region->base_address;
    region->memory[addr] = value & 0xFF;
    region->memory[addr + 1] = (value >> 8) & 0xFF;
}

uint16_t MPU::read16(uint32_t addr) {
    MemoryRegion *region = getRegion(addr);
    //return *(uint16_t*)&region->memory[addr - region->base_address];
    addr -= region->base_address;
    return (region->memory[addr + 1] << 8) | (region->memory[addr]);
}

void MPU::write32(uint32_t addr, uint32_t value) {
    MemoryRegion *region = getRegion(addr);
    //*(uint32_t*)&region->memory[addr - region->base_address] = value;
    addr -= region->base_address;
    region->memory[addr] = value & 0xFF;
    region->memory[addr + 1] = (value >> 8) & 0xFF;
    region->memory[addr + 2] = (value >> 16) & 0xFF;
    region->memory[addr + 3] = (value >> 24) & 0xFF;
}

uint32_t MPU::read32(uint32_t addr) {
    MemoryRegion *region = getRegion(addr);
    //return *(uint32_t*)&region->memory[addr - region->base_address];
    addr -= region->base_address;
    return (region->memory[addr + 3] << 24) |
        (region->memory[addr + 2] << 16) |
        (region->memory[addr + 1] << 8) |
        (region->memory[addr]);
}
