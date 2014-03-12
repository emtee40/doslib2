#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef TARGET_LINUX
static const char str_spc[] = " ";
static const char str_ast[] = "\x1B[33m*\x1B[0m";
#else
static const char str_spc[] = " ";
static const char str_ast[] = "*";
#endif

#pragma pack(push,1)
typedef struct tf8086_record {
	uint16_t			r_recid;
	uint16_t			r_reclen;
	uint16_t			r_di;
	uint16_t			r_si;
	uint16_t			r_bp;
	uint16_t			r_sp;
	uint16_t			r_bx;
	uint16_t			r_dx;
	uint16_t			r_cx;
	uint16_t			r_ax;
	uint16_t			r_flags;
	uint16_t			r_ip;
	uint16_t			r_cs;
	uint16_t			r_ss;
	uint16_t			r_ds;
	uint16_t			r_es;
	unsigned char			r_csip_capture[4];
	unsigned char			r_sssp_capture[4];
} tf8086_record; /* 40 bytes */
#pragma pack(pop)

#pragma pack(push,1)
typedef struct tf286_record {
	uint16_t			r_recid;
	uint16_t			r_reclen;
	uint16_t			r_di;
	uint16_t			r_si;
	uint16_t			r_bp;
	uint16_t			r_sp;
	uint16_t			r_bx;
	uint16_t			r_dx;
	uint16_t			r_cx;
	uint16_t			r_ax;
	uint16_t			r_flags;
	uint16_t			r_ip;
	uint16_t			r_cs;
	uint16_t			r_ss;
	uint16_t			r_ds;
	uint16_t			r_es;
	unsigned char			r_csip_capture[4];
	unsigned char			r_sssp_capture[4];
	uint16_t			r_msw;
} tf286_record; /* 42 bytes */
#pragma pack(pop)

#pragma pack(push,1)
typedef struct tf386_record {
	uint16_t			r_recid;
	uint16_t			r_reclen;
	uint32_t			r_edi;
	uint32_t			r_esi;
	uint32_t			r_ebp;
	uint32_t			r_esp;
	uint32_t			r_ebx;
	uint32_t			r_edx;
	uint32_t			r_ecx;
	uint32_t			r_eax;
	uint32_t			r_eflags;
	uint32_t			r_eip;
	uint32_t			r_cr0;
	uint32_t			r_cr2;
	uint32_t			r_cr3;
	uint32_t			r_cr4;
	uint32_t			r_dr0;
	uint32_t			r_dr1;
	uint32_t			r_dr2;
	uint32_t			r_dr3;
	uint32_t			r_dr6;
	uint32_t			r_dr7;
	uint16_t			r_cs;
	uint16_t			r_ss;
	uint16_t			r_ds;
	uint16_t			r_es;
	uint16_t			r_fs;
	uint16_t			r_gs;
	unsigned char			r_csip_capture[4];
	unsigned char			r_sssp_capture[4];
} tf386_record; /* 104 bytes */
#pragma pack(pop)

static unsigned char buffer[512];

static void dump_386(int fd) {
	struct tf386_record *rec,prec;
	int rd;

	memset(&prec,0xFF,sizeof(prec));
	while ((rd=read(fd,buffer,4)) == 4) {
		rec = (struct tf386_record*)buffer;

		if (rec->r_reclen < 4) {
			fprintf(stderr,"Invalid record type=0x%04x length=0x%04x\n",rec->r_recid,rec->r_reclen);
			break;
		}

		if (rec->r_recid == 0x8386) {
			if (rec->r_reclen != 104) {
				fprintf(stderr,"WARNING: Invalid 386 record len=0x%04x\n",rec->r_reclen);
				lseek(fd,rec->r_reclen-4,SEEK_CUR);
				continue;
			}

			read(fd,buffer+4,104-4);
			printf("[286] CS:IP 0x%04x%s:0x%04x%s [0x%02x 0x%02x 0x%02x 0x%02x] FLAGS=0x%04x%s\n",
					rec->r_cs,	(rec->r_cs == prec.r_cs)?str_spc:str_ast,
					rec->r_eip,	(rec->r_eip == prec.r_eip)?str_spc:str_ast,
					rec->r_csip_capture[0],rec->r_csip_capture[1],
					rec->r_csip_capture[2],rec->r_csip_capture[3],
					rec->r_eflags,	(rec->r_eflags == prec.r_eflags)?str_spc:str_ast);
			printf("       AX=%04x%s BX=%04x%s CX=%04x%s DX=%04x%s SI=%04x%s DI=%04x%s BP=%04x%s\n",
					rec->r_eax,	(rec->r_eax == prec.r_eax)?str_spc:str_ast,
					rec->r_ebx,	(rec->r_ebx == prec.r_ebx)?str_spc:str_ast,
					rec->r_ecx,	(rec->r_ecx == prec.r_ecx)?str_spc:str_ast,
					rec->r_edx,	(rec->r_edx == prec.r_edx)?str_spc:str_ast,
					rec->r_esi,	(rec->r_esi == prec.r_esi)?str_spc:str_ast,
					rec->r_edi,	(rec->r_edi == prec.r_edi)?str_spc:str_ast,
					rec->r_ebp,	(rec->r_ebp == prec.r_ebp)?str_spc:str_ast);
			printf("       DS=%04x%s ES=%04x%s SS:SP 0x%04x%s:%04x%s [0x%02x 0x%02x 0x%02x 0x%02x]\n",
					rec->r_ds,	(rec->r_ds == prec.r_ds)?str_spc:str_ast,
					rec->r_es,	(rec->r_es == prec.r_es)?str_spc:str_ast,
					rec->r_ss,	(rec->r_ss == prec.r_ss)?str_spc:str_ast,
					rec->r_esp,	(rec->r_esp == prec.r_esp)?str_spc:str_ast,
					rec->r_sssp_capture[0],rec->r_sssp_capture[1],
					rec->r_sssp_capture[2],rec->r_sssp_capture[3]);

			prec = *rec;
		}
		else {
			fprintf(stderr,"WARNING: Unknown record type=0x%04x len=0x%04x\n",rec->r_recid,rec->r_reclen);
			lseek(fd,rec->r_reclen-4,SEEK_CUR);
		}
	}
}

static void dump_286(int fd) {
	struct tf286_record *rec,prec;
	int rd;

	memset(&prec,0xFF,sizeof(prec));
	while ((rd=read(fd,buffer,4)) == 4) {
		rec = (struct tf286_record*)buffer;

		if (rec->r_reclen < 4) {
			fprintf(stderr,"Invalid record type=0x%04x length=0x%04x\n",rec->r_recid,rec->r_reclen);
			break;
		}

		if (rec->r_recid == 0x8286) {
			if (rec->r_reclen != 42) {
				fprintf(stderr,"WARNING: Invalid 286 record len=0x%04x\n",rec->r_reclen);
				lseek(fd,rec->r_reclen-4,SEEK_CUR);
				continue;
			}

			read(fd,buffer+4,42-4);
			printf("[286] CS:IP 0x%04x%s:0x%04x%s [0x%02x 0x%02x 0x%02x 0x%02x] FLAGS=0x%04x%s\n",
					rec->r_cs,	(rec->r_cs == prec.r_cs)?str_spc:str_ast,
					rec->r_ip,	(rec->r_ip == prec.r_ip)?str_spc:str_ast,
					rec->r_csip_capture[0],rec->r_csip_capture[1],
					rec->r_csip_capture[2],rec->r_csip_capture[3],
					rec->r_flags,	(rec->r_flags == prec.r_flags)?str_spc:str_ast);
			printf("       AX=%04x%s BX=%04x%s CX=%04x%s DX=%04x%s SI=%04x%s DI=%04x%s BP=%04x%s\n",
					rec->r_ax,	(rec->r_ax == prec.r_ax)?str_spc:str_ast,
					rec->r_bx,	(rec->r_bx == prec.r_bx)?str_spc:str_ast,
					rec->r_cx,	(rec->r_cx == prec.r_cx)?str_spc:str_ast,
					rec->r_dx,	(rec->r_dx == prec.r_dx)?str_spc:str_ast,
					rec->r_si,	(rec->r_si == prec.r_si)?str_spc:str_ast,
					rec->r_di,	(rec->r_di == prec.r_di)?str_spc:str_ast,
					rec->r_bp,	(rec->r_bp == prec.r_bp)?str_spc:str_ast);
			printf("       DS=%04x%s ES=%04x%s MSW=0x%04x SS:SP 0x%04x%s:%04x%s [0x%02x 0x%02x 0x%02x 0x%02x]\n",
					rec->r_ds,	(rec->r_ds == prec.r_ds)?str_spc:str_ast,
					rec->r_es,	(rec->r_es == prec.r_es)?str_spc:str_ast,
					rec->r_msw,
					rec->r_ss,	(rec->r_ss == prec.r_ss)?str_spc:str_ast,
					rec->r_sp,	(rec->r_sp == prec.r_sp)?str_spc:str_ast,
					rec->r_sssp_capture[0],rec->r_sssp_capture[1],
					rec->r_sssp_capture[2],rec->r_sssp_capture[3]);

			prec = *rec;
		}
		else {
			fprintf(stderr,"WARNING: Unknown record type=0x%04x len=0x%04x\n",rec->r_recid,rec->r_reclen);
			lseek(fd,rec->r_reclen-4,SEEK_CUR);
		}
	}
}

static void dump_8086(int fd) {
	struct tf8086_record *rec,prec;
	int rd;

	memset(&prec,0xFF,sizeof(prec));
	while ((rd=read(fd,buffer,4)) == 4) {
		rec = (struct tf8086_record*)buffer;

		if (rec->r_reclen < 4) {
			fprintf(stderr,"Invalid record type=0x%04x length=0x%04x\n",rec->r_recid,rec->r_reclen);
			break;
		}

		if (rec->r_recid == 0x8086) {
			if (rec->r_reclen != 40) {
				fprintf(stderr,"WARNING: Invalid 8086 record len=0x%04x\n",rec->r_reclen);
				lseek(fd,rec->r_reclen-4,SEEK_CUR);
				continue;
			}

			read(fd,buffer+4,40-4);
			printf("[8086] CS:IP 0x%04x%s:0x%04x%s [0x%02x 0x%02x 0x%02x 0x%02x] FLAGS=0x%04x%s\n",
					rec->r_cs,	(rec->r_cs == prec.r_cs)?str_spc:str_ast,
					rec->r_ip,	(rec->r_ip == prec.r_ip)?str_spc:str_ast,
					rec->r_csip_capture[0],rec->r_csip_capture[1],
					rec->r_csip_capture[2],rec->r_csip_capture[3],
					rec->r_flags,	(rec->r_flags == prec.r_flags)?str_spc:str_ast);
			printf("       AX=%04x%s BX=%04x%s CX=%04x%s DX=%04x%s SI=%04x%s DI=%04x%s BP=%04x%s\n",
					rec->r_ax,	(rec->r_ax == prec.r_ax)?str_spc:str_ast,
					rec->r_bx,	(rec->r_bx == prec.r_bx)?str_spc:str_ast,
					rec->r_cx,	(rec->r_cx == prec.r_cx)?str_spc:str_ast,
					rec->r_dx,	(rec->r_dx == prec.r_dx)?str_spc:str_ast,
					rec->r_si,	(rec->r_si == prec.r_si)?str_spc:str_ast,
					rec->r_di,	(rec->r_di == prec.r_di)?str_spc:str_ast,
					rec->r_bp,	(rec->r_bp == prec.r_bp)?str_spc:str_ast);
			printf("       DS=%04x%s ES=%04x%s SS:SP 0x%04x%s:%04x%s [0x%02x 0x%02x 0x%02x 0x%02x]\n",
					rec->r_ds,	(rec->r_ds == prec.r_ds)?str_spc:str_ast,
					rec->r_es,	(rec->r_es == prec.r_es)?str_spc:str_ast,
					rec->r_ss,	(rec->r_ss == prec.r_ss)?str_spc:str_ast,
					rec->r_sp,	(rec->r_sp == prec.r_sp)?str_spc:str_ast,
					rec->r_sssp_capture[0],rec->r_sssp_capture[1],
					rec->r_sssp_capture[2],rec->r_sssp_capture[3]);

			prec = *rec;
		}
		else {
			fprintf(stderr,"WARNING: Unknown record type=0x%04x len=0x%04x\n",rec->r_recid,rec->r_reclen);
			lseek(fd,rec->r_reclen-4,SEEK_CUR);
		}
	}
}

int main(int argc,char **argv) {
	struct tf8086_record *rec;
	int fd,rd;

	assert(sizeof(*rec) == 40);
	assert(sizeof(struct tf286_record) == 42);
	assert(sizeof(struct tf386_record) == 104);

	if (argc < 2) {
		fprintf(stderr,"tflogdump <file>\n");
		return 1;
	}

	fd = open(argv[1],O_RDONLY|O_BINARY);
	if (fd < 0) {
		fprintf(stderr,"Cannot open for reading %s, %s\n",argv[1],strerror(errno));
		return 1;
	}

	rec = (struct tf8086_record*)buffer;
	if ((rd=read(fd,buffer,4)) != 4)
		return 1;

	lseek(fd,0,SEEK_SET);
	if (rec->r_recid == 0x8086)
		dump_8086(fd);
	else if (rec->r_recid == 0x8286)
		dump_286(fd);
	else if (rec->r_recid == 0x8386)
		dump_386(fd);
	else
		fprintf(stderr,"Unknown record type 0x%04x\n",rec->r_recid);

	close(fd);
	return 0;
}

