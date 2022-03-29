#ifndef ELF_FILE_H
#define ELF_FILE_H
#include <stdint.h>

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;	
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;


#define ELF32_ST_BIND(val)		(((uint8_t) (val)) >> 4)
#define ELF32_ST_TYPE(val)		((val) & 0xf)
#define ELF32_ST_INFO(bind, type)	(((bind) << 4) + ((type) & 0xf))

#define ELF32_R_SYM(val)		((val) >> 8)
#define ELF32_R_TYPE(val)		((val) & 0xff)
#define ELF32_R_INFO(sym, type)	(((sym) << 8) + ((type) & 0xff))

#define ELF_NIDENT	16

typedef struct {
	uint8_t e_ident[ELF_NIDENT];
	Elf32_Half e_type;
	Elf32_Half e_machine;
	Elf32_Word e_version;
	Elf32_Addr e_entry;
	Elf32_Off e_phoff;
	Elf32_Off e_shoff;
	Elf32_Word e_flags;
	Elf32_Half e_ehsize;
	Elf32_Half e_phentsize;
	Elf32_Half e_phnum;
	Elf32_Half e_shentsize;
	Elf32_Half e_shnum;
	Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct {
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off sh_offset;
	Elf32_Word sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Word sh_addralign;
	Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
	Elf32_Word st_name;
	Elf32_Addr st_value;
	Elf32_Word st_size;
	uint8_t	 st_info;
	uint8_t	 st_other;
	Elf32_Half st_shndx;
} Elf32_Sym;

typedef struct {
	Elf32_Addr r_offset;		
	Elf32_Word r_info;	
} Elf32_Rel;

typedef struct {
	Elf32_Addr r_offset;
	Elf32_Word r_info;	
	Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct {
	Elf32_Word p_type;		
	Elf32_Off p_offset;	
	Elf32_Addr p_vaddr;	
	Elf32_Addr p_paddr;	
	Elf32_Word p_filesz;
	Elf32_Word p_memsz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
} Elf32_Phdr;

enum Elf_Ident {
	EI_MAG0 = 0, 
	EI_MAG1, 
	EI_MAG2, 
	EI_MAG3, 
	EI_CLAS, 
	EI_DATA, 
	EI_VERSIO, 
	EI_OSABI, 
	EI_ABIVERS,
	EI_PAD 
};
 
enum SectionSpecialIndexes {
	SHN_UNDEF = 0,
	SHN_LORESERVE = 0xFF00,
	SHN_LOPROC = 0xFF00,
	SHN_HIPROC = 0xff1f,
	SHN_ABS = 0xfff1,
	SHN_COMMON = 0xfff2,
	SHN_HIRESERVE = 0xffff
};

enum ShT_Types {
	SHT_NULL = 0, 
	SHT_PROGBITS,
	SHT_SYMTAB,
	SHT_STRTAB,
	SHT_RELA,
	SHT_NOBITS,
	SHT_REL
};

enum ShT_Attributes {
	SHF_WRITE     = 0x01,
	SHF_ALLOC     = 0x02,
	SHF_EXECINSTR = 0x04
};

enum SegmentTypes {
	PT_NULL = 0,
	PT_LOAD,
	PT_DYNAMIC,
	PT_INTERP,
	PT_NOTE,
	PT_SHLIB,
	PT_PHDR,
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7fffffff
};

#endif
