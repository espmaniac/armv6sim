#include <cstdint>
#include <string.h>
#include "mpu.hpp"

class ARM {
public:
	ARM();
	~ARM();

	void decodeDataProcessing(uint32_t);

	void decodeLoadAndStore(uint32_t);

	void decodeBranch(uint32_t);
	
	void decodeMultiply(uint32_t);

	void execute();
	
	bool shifter(uint8_t, uint32_t, uint32_t&, bool);

	bool condition(uint32_t);
	
	bool getFlag(uint8_t) const;

	void setFlag(uint8_t, bool);

	uint32_t readRegister(uint8_t) const;

	void setRegister(uint8_t, uint32_t);

	uint32_t readPc() const;
	
	void setPc(uint32_t);
	
	MPU *getMpu() const;
	
	//void decodeMediaInstructions(uint32_t instruction);

	uint32_t signExtend(uint32_t, uint8_t, uint8_t);

private:
	uint32_t registers[17]; // registers[16] - CPSR
	MPU mpu;
	bool usedPc;

	enum { // CPSR
		T = 5,
		Q = 27,
		V = 28,
		C = 29,
		Z = 30,
		N = 31
	};

};
