#include <stdio.h>
#include <string>
#include <fstream>
#include <malloc.h>
#include "arm.hpp"
#include "elf.h"

int main() {
	ARM arm;
	std::ifstream file("tests/a.out", std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	char* payload = new char[size]{0};
	file.read(payload, size);
	Elf32_Ehdr *Eheader = (Elf32_Ehdr*)payload;
	Elf32_Phdr *Pheader = (Elf32_Phdr*)(payload + Eheader->e_phoff);
	void *entry_point = nullptr;
	
	for (uint32_t i = 0; i < Eheader->e_phnum; ++i) {
		if (Pheader[i].p_type == PT_LOAD) {
			arm.getMpu()->createRegion(Pheader[i].p_paddr, Pheader[i].p_memsz);
			void* data = arm.getMpu()->getRegion(Pheader[i].p_paddr)->memory;
			memcpy(data, (void*)(payload + Pheader[i].p_offset), Pheader[i].p_filesz);
		}
	}
	arm.getMpu()->createRegion(0, Eheader->e_entry);
	arm.setRegister(13, Eheader->e_entry);
	arm.setPc(Eheader->e_entry);
	arm.decode();

	delete[] payload;
	file.close();
	return 0;
}
