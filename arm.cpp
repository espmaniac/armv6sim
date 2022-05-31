#include "arm.hpp"

ARM::ARM() : registers{0}, usedPc(false) {}
ARM::~ARM() {}

void ARM::decodeDataProcessing(uint32_t instruction) {
	const uint8_t Rm = instruction & 0xF;
	const uint8_t Rd = (instruction >> 12) & 0xF;
	const uint32_t Rn = readRegister((instruction >> 16) & 0xF);
	uint32_t shifter_operand = readRegister(Rm);
	bool shifter_carry_out = false;

	if (((instruction >> 25) & 0x7) == 0b001) { // 32bit immidiate 
		const uint16_t immed_8 = instruction & 0xFF;
		const uint16_t rotate_imm = ((instruction >> 8) & 0xF) * 2;
		shifter_operand = (immed_8 >> rotate_imm) | (immed_8 << (32 -  rotate_imm));
		shifter_carry_out = ((instruction & 0xFF) == 0) ? getFlag(C) : (shifter_operand >> 31) & 1;
	} else if (((instruction >> 25) & 0x7) == 0b000) {
		if ((instruction & (1 << 7)) && (instruction & (1 << 4)))
			return;
		uint32_t shift = (instruction & (1 << 4)) ? (readRegister((instruction >> 8) & 0xF) & 0x7F): ((instruction >> 7) & 0x1F);
		shifter_carry_out = shifter((instruction >> 5) & 0x3, shift, shifter_operand, instruction & (1 << 4));
	} else
		return;
	
	bool C_FLAG = shifter_carry_out;
	bool V_FLAG = getFlag(V);

	switch((instruction >> 21) & 0xF) {
		case 0b0000: // AND
			setRegister(Rd, Rn & shifter_operand);
			break;
		
		case 0b0001: // EOR
			setRegister(Rd, Rn ^ shifter_operand);
			break;

		case 0b0010: // SUB
			setRegister(Rd, Rn - shifter_operand);
			//C_FLAG = (((Rn >> 31) - (shifter_operand >> 31) - (registers[Rd] >> 31)) <= 1) ;
			C_FLAG = !(((Rn >> 31) - (shifter_operand >> 31) - (readRegister(Rd) >> 31)) & (1 << 31)) ;
			V_FLAG = ((Rn >> 31) != (shifter_operand >> 31)) && ((Rn >> 31) != (readRegister(Rd) >> 31));
			break;	

		case 0b0011: // RSB
			setRegister(Rd, shifter_operand - Rn);
			//C_FLAG = (((shifter_operand >> 31) - (Rn >> 31) - (registers[Rd] >> 31)) <= 1);
			C_FLAG = !(((shifter_operand >> 31) - (Rn >> 31) - (readRegister(Rd) >> 31)) & (1 << 31));
			V_FLAG = ((Rn >> 31) != (shifter_operand >> 31)) && ((shifter_operand >> 31) != (readRegister(Rd) >> 31));
			break;

		case 0b0100: // ADD
			setRegister(Rd, Rn + shifter_operand);
			C_FLAG = ((Rn >> 31) + (shifter_operand >> 31)) > (readRegister(Rd) >> 31);
			V_FLAG = ((Rn >> 31) == (shifter_operand >> 31)) && ((Rn >> 31) != (readRegister(Rd) >> 31));
			break;

		case 0b0101: // ADC
			setRegister(Rd, Rn + shifter_operand + (getFlag(C)));
			C_FLAG = ((Rn >> 31) + (shifter_operand >> 31)) > (readRegister(Rd) >> 31);
			V_FLAG = ((Rn >> 31) == (shifter_operand >> 31)) && ((Rn >> 31) != (readRegister(Rd) >> 31));
			break;

		case 0b0110: // SBC
			setRegister(Rd, Rn - shifter_operand - !(getFlag(C)));
			C_FLAG = !(((Rn >> 31) - (shifter_operand >> 31) - (readRegister(Rd) >> 31)) & (1 << 31));
			V_FLAG = ((Rn >> 31) != (shifter_operand >> 31)) && ((Rn >> 31) != (readRegister(Rd) >> 31));
			break;

		case 0b0111: // RSC
			setRegister(Rd, shifter_operand - Rn - !(getFlag(C)));
			C_FLAG = !(((shifter_operand >> 31) - (Rn >> 31) - (readRegister(Rd) >> 31)) & (1 << 31));
			V_FLAG = ((Rn >> 31) != (shifter_operand >> 31)) && ((shifter_operand >> 31) != (readRegister(Rd) >> 31));
			break;

		case 0b1000: { // TST
			if (!(instruction & (1 << 20)) && ((instruction >> 12) & 0xF) != 0)
				return;
			const uint32_t alu_out = Rn & shifter_operand;
			setFlag(N, alu_out & (1 << 31));
			setFlag(Z, alu_out == 0);
			setFlag(C, shifter_carry_out);
			return;
		}

		case 0b1001: { // TEQ
			if (!(instruction & (1 << 20)) && ((instruction >> 12) & 0xF) != 0)
				return;
			const uint32_t alu_out = Rn ^ shifter_operand;
			setFlag(C, shifter_carry_out);
			setFlag(N, alu_out & (1 << 31));
			setFlag(Z, alu_out == 0);
			return;
		}

		case 0b1010: { // CMP
			if (!(instruction & (1 << 20)) && ((instruction >> 12) & 0xF) != 0)
				return;
			const uint32_t alu_out = Rn - shifter_operand; 
			//setC( (((Rn >> 31) - (shifter_operand >> 31) - (alu_out >> 31)) <= 1) );
			setFlag(C, !(((Rn >> 31) - (shifter_operand >> 31) - (alu_out >> 31)) & (1 << 31)));
			setFlag(N, alu_out & (1 << 31));
			setFlag(Z, alu_out == 0);
			setFlag(V, ((Rn >> 31) != (shifter_operand >> 31)) && ((Rn >> 31) != (alu_out >> 31)));
			return;
		}

		case 0b1011: { // CMN or CLZ
			if (!(instruction & (1 << 20))) {
				if ((((instruction >> 8) & 0xF) == 0xF) && (((instruction >> 16) & 0xF) == 0xF)) { // CLZ
					uint8_t highestSetBit = 31;
					for (; (highestSetBit > 0) && !((readRegister(Rm) >> highestSetBit) & 1); --highestSetBit);
					setRegister(Rd, (readRegister(Rm) == 0) ? 32 : 32 - (highestSetBit + 1));
					return;
				} else if (((instruction >> 12) & 0xF) != 0) // not (CMN or CLZ)
					return;
			}

			const uint32_t alu_out = Rn + shifter_operand;
			setFlag(N, alu_out & (1 << 31));
			setFlag(Z, alu_out == 0);
			setFlag(C, ((Rn >> 31) + (shifter_operand >> 31)) > (alu_out >> 31));
			setFlag(V, ((Rn >> 31) == (shifter_operand >> 31)) && ((Rn >> 31) != (alu_out >> 31)));
			return;
		}
		
		case 0b1100: // ORR
			setRegister(Rd, Rn | shifter_operand);	
			break;

		case 0b1101: // MOV or CPY(pseudo-instruction)
			if (((instruction >> 16) & 0xF) != 0)
				return;
			setRegister(Rd, shifter_operand);
			break;

		case 0b1110: // BIC
			setRegister(Rd, Rn & ~shifter_operand);
			break;

		case 0b1111: // MVN
			if (((instruction >> 16) & 0xF) != 0)
				return;
			setRegister(Rd, ~shifter_operand);
			break;

		default: break;
	}

	if (instruction & (1 << 20)) {
		setFlag(C, C_FLAG);
		setFlag(V, V_FLAG);
		setFlag(N, readRegister(Rd) & (1 << 31));
		setFlag(Z, readRegister(Rd) == 0);
	}
}

bool ARM::shifter(uint8_t shiftCase, uint32_t shift, uint32_t& shifter_operand, bool imm_or_reg) {
	const uint32_t Rm = shifter_operand;
	bool shifter_carry_out = false;
	switch (shiftCase & 0x3) {
		case 0b00: /*LSL*/
			if (shift == 0) 
				shifter_carry_out = getFlag(C);
			else if (shift < 32) {
				shifter_operand <<= shift;	
				shifter_carry_out = Rm & (1 << ((32 - shift))); 
			} else if (shift == 32) {
				shifter_operand = 0;
				shifter_carry_out = Rm & 1;
			} else {
				shifter_operand = 0;
				shifter_carry_out = 0;
			}
			break;

		case 0b01: /*LSR*/
			if (shift == 0) {
				if (imm_or_reg == false) {
					shifter_operand = 0;
					shifter_carry_out = Rm & (1 << 31);
				} 
				else
					shifter_carry_out = getFlag(C);
			}
			else if (shift < 32) {
				shifter_operand >>= shift;
				shifter_carry_out = Rm & (1 << (shift - 1));
			}
			else if (shift == 32) {
				shifter_operand = 0;
				shifter_carry_out = Rm & (1 << 31);
			}
			else if (shift > 32) {
				shifter_operand = 0;
				shifter_carry_out = 0;
			}

			break;
		case 0b10: /*ASR*/
			if (shift == 0) {
				if (imm_or_reg == false) {
					shifter_operand = (Rm & (1 << 31)) ? 0xFFFFFFFF : 0;
					shifter_carry_out = Rm & (1 << 31);
				} 
				else 
					shifter_carry_out = getFlag(C);
			} else if (shift < 32) {
				shifter_operand = (int32_t)Rm >> shift;
				shifter_carry_out = Rm & (1 << (shift - 1));
			} else if (shift >= 32) {
				shifter_operand = (Rm & (1 << 31)) ? 0xFFFFFFFF : 0;
				shifter_carry_out = Rm & (1 << 31);
			}
			break;
		case 0b11: /*ROR or RRX*/
			if (shift == 0) {
				if (imm_or_reg == false) {
					shifter_operand = ((getFlag(C)) << 31) | (Rm >> 1);
					shifter_carry_out = Rm & 1;
				} 
				else 
					shifter_carry_out = ((shift & 0xF) == 0) ? Rm & (1 << 31) : getFlag(C);
			} else if (shift > 0) {
				if (imm_or_reg == false) {
					shifter_operand = (Rm >> shift) | (Rm << (32 - shift));
					shifter_carry_out = Rm & (1 << (shift - 1));
				} else
					if ((shift & 0xF) > 0) {
						shifter_operand = (Rm >> (shift & 0xF)) | (Rm << (32 - (shift & 0xF)));
						shifter_carry_out = Rm & (1 << ((shift & 0xF) - 1));
					}
			}
								
			break;

		default: break;
	}

	return shifter_carry_out;
}

void ARM::decodeLoadAndStore(uint32_t instruction) {
	const uint8_t Rn = (instruction >> 16) & 0xF; // contains base address
	const uint8_t Rd = (instruction >> 12) & 0xF;
	const uint8_t Rm = instruction & 0xF;
	uint32_t address = readRegister(Rn);

	if (((instruction >> 26) & 0x3) == 0b01) { // LOAD AND STORE
		uint32_t offset = (instruction & (1 << 25)) ? readRegister(Rm) : (instruction & 0xFFF);
		if (instruction & (1 << 25) && (((instruction >> 4) & 0xFF) != 0)) // shift
			shifter((instruction >> 5) & 0x3, (instruction >> 7) & 0x1F, offset, true);

		if (instruction & (1 << 24)) // P bit pre-indexed
			address += (instruction & (1 << 23)) ? offset : -offset; // U Bit

		if (instruction & (1 << 20)) { // Load
			setRegister(Rd, (instruction & (1 << 22)) ? mpu.read8(address) : mpu.read32(address)); 
			/*
			const uint32_t rotate = 8 * (address & 0x3);
			const uint32_t data = mpu.read32(address);
			registers[Rd] = (instruction & (1 << 22)) ? mpu.read8(address) : (instruction & (1 << 23)) ?
				data : ((data >> rotate) | (data << (32 - rotate)));
			*/
		}
		else { // Store
			if (instruction & (1 << 22)) // STRB
				mpu.write8(address, readRegister(Rd));
			else // STR
				mpu.write32(address, readRegister(Rd));
		}

		if (!(instruction & (1 << 24))) // P bit post-indexed
			setRegister(Rn, address += (instruction & (1 << 23)) ? offset : -offset); // U Bit

	} else if ((((instruction >> 25) & 0x7) == 0b000) && (instruction & (1 << 7)) && (instruction & (1 << 4))) { // Miscellaneous
		uint32_t offset = 0;
		
		if(instruction & (1 << 22)) // immidiate
			offset = ((((instruction >> 8) & 0xF) << 4) | (instruction & 0xF));
		else {
			if (((instruction >> 8) & 0xF) != 0) return;
			offset = readRegister(Rm);
		}
		
		if (instruction & (1 << 24))
			address += (instruction & (1 << 23)) ? offset : -offset;

		switch (((((instruction>>20) & 0x1)<<2) | ((instruction>>5) & 0x3)) & 0x7) { // LSH bits
			case 0b000: { // SWP
				const uint32_t data = mpu.read32(readRegister(Rn));
				const uint32_t rotate = 8 * (readRegister(Rn) & 0x3);
				const uint32_t temp = (instruction & (1 << 22)) ? mpu.read8(readRegister(Rn)) : (instruction & (1 << 23))?
					data : ((data >> rotate) | (data << (32 - rotate)));
				if (instruction & (1 << 22))
					mpu.write8(readRegister(Rn), readRegister(Rm));
				else
					mpu.write32(readRegister(Rn), readRegister(Rm));
				setRegister(Rd, temp);
				break;
			}
			case 0b001: // Store halfword STRH
				mpu.write16(address, readRegister(Rd));
				break;

			case 0b010: // Load doubleword LDRD
				setRegister(Rd, mpu.read32(address));
				setRegister(Rd + 1, mpu.read32(address + 4));
				break;

			case 0b011: // Store doubleword STRD
				mpu.write32(address, readRegister(Rd));
				mpu.write32(address + 4, readRegister(Rd + 1));
				break;

			case 0b101: // Load unsigned halfword LDRH
				setRegister(Rd, mpu.read16(address));
				break;

			case 0b110: // Load signed byte LDRSB
				setRegister(Rd, signExtend(mpu.read8(address), 8, 32));
				break;

			case 0b111: // Load signed halfword LDRSH
				setRegister(Rd, signExtend(mpu.read16(address), 16, 32));
				break;

			default: break;
		}

		if (!(instruction & (1 << 24)))
			setRegister(Rn, address += (instruction & (1 << 23)) ? offset : -offset);

	} else if (((instruction >> 25) & 0x7) == 0b100) { // Multiple Load or Store
		for (uint8_t i = 0; i < 16; ++i) {
			const uint32_t reg = (instruction & (1 << 23)) ? i : (15 - i);
			if (instruction & (1 << reg)) {
				if (instruction & (1 << 24))
					address += (instruction & (1 << 23)) ? 4 : -4;
				
				if (instruction & (1 << 20))
					setRegister(reg, mpu.read32(address));
				else
					mpu.write32(address, readRegister(reg));

				if (!(instruction & (1 << 24)))
					address += (instruction & (1 << 23)) ? 4 : -4;
			}
		}
	} else return;

	if ((instruction & (1 << 21))) // W bit
		setRegister(Rn, address);
}

void ARM::decodeBranch(uint32_t instruction) {
	if (((instruction >> 25) & 0x7) == 0b101) { // B, BL, BLX(1)
		if (((instruction >> 28) & 0xF) == 0b1111) { // BLX
			setFlag(T, 1);
			setRegister(14, readPc() + 4); // next instruction pc + 4
			setPc(readPc() + (((instruction >> 24) & 1) << 1));
		}
		else {// B, BL
			if (instruction & (1 << 24)) // L bit 
				setRegister(14, readPc() + 4); // next instruction
		}
		setPc(readPc() + (signExtend(((instruction & 0xFFFFFF) << 2), 26, 32)) + 8);
	} else if (((instruction >> 20) & 0xFF) == 0b00010010) { // BX BLX(2)
		const uint16_t Rm = instruction & 0xF;
		if (((instruction >> 4) & 0xF) == 0b0011) // BLX(2)
			setRegister(14, readPc() + 4); // next instruction
		setRegister(15, readRegister(Rm)); // T flag = value & 1;
	}
}

void ARM::decodeMultiply(uint32_t instruction) {
	if ((((instruction >> 24) & 0xF) != 0) || !(instruction & (1 << 7)) || !(instruction & (1 << 4))) 
		return;

	const uint8_t Rd = (instruction >> 16) & 0xF;
	const uint8_t Rn = (instruction >> 12) & 0xF;
	const uint8_t Rs = (instruction >> 8) & 0xF;
	const uint8_t Rm = instruction & 0xF;

	bool Z_FLAG = getFlag(Z);

	if ((((instruction >> 21) & 0x7) == 0) && (((instruction >> 12) & 0xF) == 0)) { // MUL
		setRegister(Rd, readRegister(Rm) * (readRegister(Rs)));
		Z_FLAG = (bool)(readRegister(Rd) == 0);
	} else if (((instruction >> 21) & 0x7) == 0b001) { // MLA
		setRegister(Rd, readRegister(Rm) * readRegister(Rs) + readRegister(Rn));
		Z_FLAG = (bool)(readRegister(Rd) == 0);
	} else if (((instruction >> 20) & 0xF) == 0b0100) { // UMAAL
		const uint64_t result = (uint64_t)readRegister(Rm) * (uint64_t)readRegister(Rs) + 
			(uint64_t)readRegister(Rn) + (uint64_t)readRegister(Rd);

		setRegister(Rn, result & 0xFFFFFFFF);
		setRegister(Rd, (result>>32) & 0xFFFFFFFF);
	} else if (((instruction >> 21) & 0x7) == 0b101) { // UMLAL
		const uint64_t result = (uint64_t)readRegister(Rm) * (uint64_t)readRegister(Rs);
		setRegister(Rd, ((result >> 32) & 0xFFFFFFFF) + readRegister(Rd) + 
			(bool)((((result >> 31) & 1) + (readRegister(Rn) >> 31)) > ((((result & 0xFFFFFFFF) + readRegister(Rn)) >> 31) & 1)));
		setRegister(Rn, (result & 0xFFFFFFFF) + readRegister(Rn));
		Z_FLAG = (bool)((readRegister(Rd) == 0) && (readRegister(Rn) == 0));
	} else if (((instruction >> 21) & 0x7) == 0b100) { // UMULL
		const uint64_t result = (uint64_t)readRegister(Rm) * (uint64_t)readRegister(Rs);
		setRegister(Rn, result & 0xFFFFFFFF);
		setRegister(Rd, (result >> 32) & 0xFFFFFFFF);
		Z_FLAG = (bool)((readRegister(Rd) == 0) && (readRegister(Rn) == 0));
	} else if (((instruction >> 21) & 0x7) == 0b110) { // SMULL
		const int64_t result = (int64_t)(int32_t)readRegister(Rm) * (int64_t)(int32_t)readRegister(Rs); 
		setRegister(Rn, result & 0xFFFFFFFF);
		setRegister(Rd, (result >> 32) & 0xFFFFFFFF);
		Z_FLAG = (bool)((readRegister(Rd) == 0) && (readRegister(Rn) == 0));
	} else if (((instruction >> 21) & 0x7) == 0b111) {
		const int64_t result = (int64_t)(int32_t)readRegister(Rm) * (int64_t)(int32_t)readRegister(Rs);
		setRegister(Rd, ((result >> 32) & 0xFFFFFFFF) + readRegister(Rd) + 
			(bool)((((result >> 31) & 1) + (readRegister(Rn) >> 31)) > ((((result & 0xFFFFFFFF) + readRegister(Rn)) >> 31) & 1)));
		setRegister(Rn, (result & 0xFFFFFFFF) + (int32_t)readRegister(Rn));
		Z_FLAG = (bool)((readRegister(Rd) == 0) && (readRegister(Rn) == 0));
	}
	if (instruction & (1 << 20)) {// S bit
		setFlag(N, readRegister(Rd) & (1 << 31));
		setFlag(Z, Z_FLAG);
	}	
}

void ARM::execute() {
	usedPc = false;
	while(registers[15] != 0/*true*/) {
		if (mpu.faultStatus()) break;
		uint32_t instruction = mpu.read32(readPc());
		if (condition(instruction)) {
			decodeDataProcessing(instruction);
			decodeLoadAndStore(instruction);
			decodeBranch(instruction);
			decodeMultiply(instruction);
			if (usedPc) {
				usedPc = false;
				continue; // decode instruction under pc now;
			}
		}
		registers[15] += 4;	
	}	
}

bool ARM::condition(uint32_t instruction) {
	switch (instruction >> 28) {
		case 0b0000: // EQ
			return getFlag(Z);
		case 0b0001: // NE
			return !getFlag(Z);
		case 0b0010: // CS/HS
			return getFlag(C);
		case 0b0011: // CC/LO
			return !getFlag(C);
		case 0b0100: // MI
			return getFlag(N);
		case 0b0101: //PL
			return !getFlag(N);
		case 0b0110: // VS
			return getFlag(V);
		case 0b0111: // VC
			return !getFlag(V);
		case 0b1000: // HI
			return getFlag(C) && !getFlag(Z);
		case 0b1001: // LS
			return !getFlag(C) || getFlag(Z);
		case 0b1010: // GE
			return getFlag(N) == getFlag(V);
		case 0b1011: // LT
			return getFlag(N) != getFlag(V);
		case 0b1100: // GT
			return !getFlag(Z) && (getFlag(N) == getFlag(V));
		case 0b1101: // LE
			return getFlag(Z) || (getFlag(N) != getFlag(V));
		case 0b1110: // AL
			return 1;
		case 0b1111:
			return 1;
		default: return 0;
	}
}

bool ARM::getFlag(uint8_t flag) const {
	return registers[16] & (1 << flag);
}

void ARM::setFlag(uint8_t flag, bool value) {
	if (value) 
		registers[16] |= (1 << flag);
	else 
		registers[16] &= ~(1 << flag);
}

uint32_t ARM::readRegister(uint8_t num) const {
	return (num == 15) ? registers[num] + 8 : registers[num];
}

void ARM::setRegister(uint8_t num, uint32_t value) {
	if (num == 15) {
		setPc(value & 0xFFFFFFFE);
		setFlag(T, value & 0x1);
	} else 
		registers[num] = value;
}

uint32_t ARM::readPc() const {
	return registers[15];
}

void ARM::setPc(uint32_t value) {
	registers[15] = value;
	usedPc = true;
}

MPU *ARM::getMpu() const {
	return (MPU*)&mpu;
}

/*
void ARM::decodeMediaInstructions(uint32_t instruction) {
	if ((((instruction >> 25) & 0x7) != 0b011) || !(instruction & (1 << 4))) return;
	const uint8_t Rm = instruction & 0xF;
	
	
	if (((instruction >> 23) & 0x3) == 0b01) {
		const uint8_t Rd = (instruction >> 12) & 0xF;
		const uint8_t Rn = (instruction >> 16) & 0xF;
		if ((((instruction >> 20) & 0x3) == 0b11) && (((instruction >> 16) & 0xF) == 0xF) && (((instruction >> 8) & 0xF) == 0xF)) { // REV REV16 REVSH
			if ((instruction & (1 << 22)) && ((instruction >> 5) & 0x7)  == 0b101) {// REVSH
				const uint16_t value = readRegister(Rm);
				setRegister(Rd, ((value >> 8) & 0x00FF) | ((value<< 8) & 0xFF00) | 
					(((value & (1 << 7)) ? 0xFFFF : 0) << 16));
			} else if (!(instruction & (1 << 22))) { // REV OR REV16
				const uint32_t value = readRegister(Rm);
				if (((instruction >> 5) & 0x7) == 0b001) { //REV
					setRegister(Rd,  ((value>>24)&0xff) | ((value<<8)&0xff0000) | 
						((value>>8)&0xff00) | ((value<<24)&0xff000000));
				} else if (((instruction >> 5) & 0x7) == 0b101) { // REV16
					setRegister(Rd, ((value >> 8) & 0xFF) | ((value & 0xFF) << 8) | 
						(((value >> 24) & 0xFF) << 16) | (((value >> 16) & 0xFf) << 24));
				}
			}
		} else if (((instruction >> 5) & 0x1F) == 0b00011) { // extend instruction space
			const uint8_t rotate = ((instruction >> 10) & 0x3) * 8;
			const uint32_t operand = readRegister(Rn);
			const uint32_t operand2 = (readRegister(Rm) >> rotate) | (readRegister(Rm) << (32 - rotate));
			switch((instruction >> 20) & 0x7) {
				case 0b000: // SXTAB16 or SXTB16
					setRegister(Rd, (signExtend(operand2 & 0xFF, 8, 16)) | 
						((signExtend((operand2 >> 16) & 0xFF, 8, 16) << 16)));

					if (Rn != 0b1111)  // SXTAB16
						setRegister(Rd, ((((readRegister(Rd) >> 16) & 0xFFFF) + ((operand >> 16) & 0xFFFF)) << 16) | 
							(((readRegister(Rd) & 0xFFFF) + (operand & 0xFFFF)) & 0xFFFF));
					
				
					break;
				case 0b010: // SXTAB or SXTB
					setRegister(Rd, signExtend(operand2 & 0xFF, 8, 32));

					if (Rn != 0b1111)  // SXTAB
						setRegister(Rd, readRegister(Rd) + operand);
					
					break;

				case 0b011: // SXTAH or SXTH
					setRegister(Rd, signExtend(operand2 & 0xFFFF, 16, 32));

					if (Rn != 0b1111) // SXTAH
						setRegister(Rd, readRegister(Rd) + operand);
					
					break;

				case 0b100: // UXTAB16 or UXTB16
					setRegister(Rd, operand2 & 0x00FF00FF);

					if (Rn != 0b1111) // UXTAB16
						setRegister(Rd, ((((readRegister(Rd) >> 16) & 0xFF) + ((operand >> 16) & 0xFFFF)) << 16) |
							(((readRegister(Rd) & 0xFFFF)) + (operand & 0xFFFF)) & 0xFFFF);
					
					break;

				case 0b110: // UXTAB or UXTB
					setRegister(Rd, operand2 & 0x000000FF);
					
					if (Rn != 0b1111) // UXTAB
						setRegister(Rd, readRegister(Rd) + operand);

					break;

				case 0b111: // UXTAH or UXTH
					setRegister(Rd, operand2 & 0x0000FFFF);

					if (Rn != 0b1111) // UXTAH
						setRegister(Rd, readRegister(Rd) + operand);
					
					break;

				default: break;
			}
		}
	} else if (((instruction >> 23) & 0x3) == 0b10) {
		const uint8_t Rd = (instruction >> 16) & 0xF;
		const uint8_t Rn = (instruction >> 12) & 0xF;
		const uint8_t Rs = (instruction >> 8) & 0xF; 
		const uint32_t operand2 = (instruction & (1 << 5)) ? 
			((readRegister(Rs) >> 16) | (readRegister(Rs) << (32 - 16))) : readRegister(Rs);

		const int32_t product1 = (int16_t)(readRegister(Rm) & 0xFFFF) * (int16_t)(operand2 & 0xFFFF);
		const int32_t product2 = (int16_t)((readRegister(Rm) >> 16) & 0xFFFF) * (int16_t)((operand2 >> 16) & 0xFFFF);
		if ((((instruction >> 20) & 0x7) == 0)) { // SMLAD or SMUAD or SMLSD or SMUSD
			if (((instruction >> 6) & 0x3) == 0) { // SMUAD or SMLAD
				if (Rn == 0b1111) { // SMUAD
					setRegister(Rd, product1 + product2);
					if (((product1 >> 31) == (product2 >> 31)) && ((product1 >> 31) != (readRegister(Rd) >> 31)))
						setFlag(Q, 1);
				} else { // SMLAD
					setRegister(Rd,(readRegister(Rn) + product1 + product2));
					if (((readRegister(Rn) >> 31) == ((uint32_t)(product1 + product2) >> 31)) && ((readRegister(Rn) >> 31) != (readRegister(Rd) >> 31)))
						setFlag(Q, 1);
				}
			} else if (((instruction >> 6) & 0x3) == 0b01) { // SMLSD or SMUSD
				if (Rn == 0b1111) // SMUSD
					setRegister(Rd, product1 - product2);
				else { // SMLSD
					setRegister(Rd, readRegister(Rn) + (product1 - product2)); // signed subtraction
					if (((readRegister(Rn) >> 31) == ((uint32_t)(product1 - product2) >> 31)) && ((readRegister(Rn) >> 31) != (readRegister(Rd) >> 31)))
						setFlag(Q, 1);
				}
			}
		} else if ((((instruction >> 20) & 0x7) == 0b100) && (((instruction >> 6) & 0x3) == 0)) { // SMLALD
			int64_t result = ((((int64_t)readRegister(Rd) & 0xFFFFFFFF) << 32) | (readRegister(Rn) & 0xFFFFFFFF));
			result += (int64_t)product1 + (int64_t)product2;		
			setRegister(Rn, result & 0xFFFFFFFF);
			setRegister(Rd, (result >> 32) & 0xFFFFFFFF);
		}
	} 
}
*/
uint32_t ARM::signExtend(uint32_t value, uint8_t bits, uint8_t extend) {
	const uint32_t mask = 1u << (bits - 1);
	return ((value ^ mask) - mask) & (uint32_t)(pow(2, extend) - 1);
}
