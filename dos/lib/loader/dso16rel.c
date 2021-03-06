/* WARNING: This code assumes 16-bit real mode */

#include <dos.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <dos/lib/loader/dso16.h>

#if defined(TARGET_MSDOS) && TARGET_BITS == 16 && defined(TARGET_REALMODE)

int ne_module_load_and_apply_segment_relocations(struct ne_module *n,unsigned int idx/*NTS: 1-based, NOT zero-based*/) {
	uint16_t offset,src_offset,src_segn;
	struct ne_segment_assign *af;
	struct ne_segment_def *df;
	unsigned char far *modp;
	unsigned char tmp[8];
	uint16_t entries;
	unsigned long o;

	if (n == NULL || idx == 0) return 1;
	if (n->ne_sega == NULL || n->ne_segd == NULL) return 1;

	df = n->ne_segd + idx - 1;
	af = n->ne_sega + idx - 1;
	if (af->segment == 0) return 1;
	if ((df->flags & 0x0006) != 0x0006) return 1; /* If not loaded or allocated, then don't apply relocations */
	if (!(df->flags & 0x100)) return 0; /* if no relocation data, bail out now */
	if (af->internal_flags & NE_SEGMENT_ASSIGN_IF_RELOC_APPLIED) return 0; /* if already applied, don't do it again */

	/* relocation data immediately follows the segment on disk */
	o = (((unsigned long)(df->offset_sectors)) << (unsigned long)n->ne_header.logical_sector_shift_count) +
		(unsigned long)df->length;

	if (lseek(n->fd,o,SEEK_SET) != o) return 1;
	if (read(n->fd,&entries,2) != 2) return 1;

	if (n->ne_module_reference_table != NULL && n->ne_header.module_reference_table_entries != 0 &&
		n->ne_header.module_reference_table_entries < 512 && n->cached_imp_mod == NULL) {
		n->cached_imp_mod = malloc(n->ne_header.module_reference_table_entries * sizeof(void*));
		if (n->cached_imp_mod == NULL) return 1;
		_fmemset(n->cached_imp_mod,0,n->ne_header.module_reference_table_entries * sizeof(void*));
	}

	while ((entries--) > 0 && read(n->fd,tmp,8) == 8) {
		offset = *((uint16_t*)(tmp+2));
		if ((offset>>4UL) >= af->length_para) continue;

		if ((tmp[1]&3) == 0) { /* internal ref */
			src_segn = tmp[4];
			if (src_segn != 0xFF) {
				src_offset = *((uint16_t*)(tmp+6));
			}
			else {
				if (n->enable_debug) fprintf(stdout,"WARNING: movable relocation entries not yet supported\n");
				continue;
			}

			if (src_segn == 0 || src_segn > n->ne_header.segment_table_entries) continue;
			src_segn = n->ne_sega[src_segn-1].segment;
		}
		else if ((tmp[1]&3) == 1) { /* import ordinal */
			unsigned int modidx = *((uint16_t*)(tmp+4));
			unsigned int ordinal = *((uint16_t*)(tmp+6));
			struct ne_module *mod;
			char modname[64];
			void far *entry;

			if (modidx == 0 || ordinal == 0) continue;
			if ((--modidx) >= n->ne_header.module_reference_table_entries) continue;

			if ((mod=n->cached_imp_mod[modidx]) == NULL) {
				if (n->import_module_lookup == NULL) {
					if (n->enable_debug) fprintf(stdout,"WARNING: Module imports symbols but caller does not provide module lookup\n");
					if (n->must_resolve_dependencies) return 1;
					continue;
				}

				if (ne_module_get_import_module_name(modname,sizeof(modname),n,modidx+1)) continue;
				if (n->enable_debug) fprintf(stdout,"Looking up module #%d %s\n",modidx+1,modname);
				if ((mod=n->cached_imp_mod[modidx]=n->import_module_lookup(n,modname)) == NULL) {
					if (n->enable_debug) fprintf(stdout,"  Failed to lookup module\n");
					if (n->must_resolve_dependencies) return 1;
					continue;
				}

				if (n->enable_debug) fprintf(stdout,"  OK, cached as %Fp\n",(void far*)mod);
				ne_module_addref(mod);
			}

			/* TODO: If the module reference did not load segments, or relocations, etc. say so here */
			if (n->import_lookup_by_ordinal != NULL) {
				if ((entry=n->import_lookup_by_ordinal(n,mod,ordinal)) == NULL) {
					if (n->enable_debug) fprintf(stdout,"WARNING: Failed to lookup ordinal %u\n",ordinal);
					if (n->must_resolve_dependencies) return 1;
					continue;
				}
			}
			else {
				if ((entry=ne_module_entry_point_by_ordinal(mod,ordinal)) == NULL) {
					if (n->enable_debug) fprintf(stdout,"WARNING: Failed to lookup ordinal %u\n",ordinal);
					if (n->must_resolve_dependencies) return 1;
					continue;
				}
			}

			src_segn = FP_SEG(entry);
			src_offset = FP_OFF(entry);

			if (n->enable_debug) fprintf(stdout,"Ordinal %u of mod %Fp -> %04x:%04x\n",
				ordinal,(void far*)mod,(unsigned int)src_segn,(unsigned int)src_offset);
		}
		else if ((tmp[1]&3) == 2) { /* import name */
			unsigned int modidx = *((uint16_t*)(tmp+4));
			unsigned int nofs = *((uint16_t*)(tmp+6));
			struct ne_module *mod;
			unsigned long length;
			char entrypoint[256];
			char modname[64];
			void far *entry;

			if (modidx == 0 || nofs == 0) continue;
			if ((--modidx) >= n->ne_header.module_reference_table_entries) continue;
			if (nofs >= n->ne_imported_names_length) continue;

			if ((mod=n->cached_imp_mod[modidx]) == NULL) {
				if (n->import_module_lookup == NULL) {
					if (n->enable_debug) fprintf(stdout,"WARNING: Module imports symbols but caller does not provide module lookup\n");
					if (n->must_resolve_dependencies) return 1;
					continue;
				}

				if (ne_module_get_import_module_name(modname,sizeof(modname),n,modidx+1)) continue;
				if (n->enable_debug) fprintf(stdout,"Looking up module #%d %s\n",modidx+1,modname);
				if ((mod=n->cached_imp_mod[modidx]=n->import_module_lookup(n,modname)) == NULL) {
					if (n->enable_debug) fprintf(stdout,"  Failed to lookup module\n");
					if (n->must_resolve_dependencies) return 1;
					continue;
				}

				if (n->enable_debug) fprintf(stdout,"  OK, cached as %Fp\n",(void far*)mod);
				ne_module_addref(mod);
			}

			/* lookup the name */
			length = n->ne_imported_names[nofs++];
			if ((nofs+length) > n->ne_imported_names_length) continue;
			if (length != 0) _fmemcpy(entrypoint,n->ne_imported_names+nofs,length);
			entrypoint[length] = 0;

			/* TODO: If the module reference did not load segments, or relocations, etc. say so here */
			if (n->import_lookup_by_name != NULL) {
				if ((entry=n->import_lookup_by_name(n,mod,entrypoint)) == NULL) {
					if (n->enable_debug) fprintf(stdout,"WARNING: Failed to lookup name %s\n",entrypoint);
					if (n->must_resolve_dependencies) return 1;
					continue;
				}
			}
			else {
				if ((entry=ne_module_entry_point_by_name(mod,entrypoint)) == NULL) {
					if (n->enable_debug) fprintf(stdout,"WARNING: Failed to lookup name %s\n",entrypoint);
					if (n->must_resolve_dependencies) return 1;
					continue;
				}
			}

			src_segn = FP_SEG(entry);
			src_offset = FP_OFF(entry);

			if (n->enable_debug) fprintf(stdout,"Name %s of mod %Fp -> %04x:%04x\n",
				entrypoint,(void far*)mod,(unsigned int)src_segn,(unsigned int)src_offset);
		}
		else {
			if (n->enable_debug) fprintf(stdout,"WARNING: This loader does not support OSFIXUP relocation data\n");
			if (n->must_resolve_dependencies) return 1;
			continue;
		}

		modp = MK_FP(af->segment,offset);

		if (tmp[1]&4) { /* bit 2 set here means we ADD the relocation, not overwrite */
			switch (tmp[0]) {
				case 0x00: /* low byte at offset */
					*((uint8_t far*)modp) += src_offset&0xFF;
					break;
				case 0x02: /* 16-bit segment */
					*((uint16_t far*)modp) += src_segn;
					break;
				case 0x03: /* 16:16 ptr */
					*((uint16_t far*)modp) += src_offset;
					*((uint16_t far*)(modp+2)) += src_segn;
					break;
				case 0x05: /* 16-bit offset */
					*((uint16_t far*)modp) += src_offset;
					break;
				case 0x0B: /* 16:32 ptr */
					*((uint32_t far*)modp) += src_offset;
					*((uint16_t far*)(modp+4)) += src_segn;
					break;
				case 0x0D: /* 32-bit offset */
					*((uint32_t far*)modp) += src_offset;
					break;
				default:
					if (n->enable_debug) fprintf(stdout,"WARNING: Reloc fixup type %u not yet supported [ADD]\n",tmp[0]);
					if (n->must_resolve_dependencies) return 1;
					break;
			}
		}
		else {
			switch (tmp[0]) {
				case 0x00: /* low byte at offset */
					*((uint8_t far*)modp) = src_offset&0xFF;
					break;
				case 0x02: /* 16-bit segment */
					*((uint16_t far*)modp) = src_segn;
					break;
				case 0x03: /* 16:16 ptr */
					*((uint16_t far*)modp) = src_offset;
					*((uint16_t far*)(modp+2)) = src_segn;
					break;
				case 0x05: /* 16-bit offset */
					*((uint16_t far*)modp) = src_offset;
					break;
				case 0x0B: /* 16:32 ptr */
					*((uint32_t far*)modp) = src_offset;
					*((uint16_t far*)(modp+4)) = src_segn;
					break;
				case 0x0D: /* 32-bit offset */
					*((uint32_t far*)modp) = src_offset;
					break;
				default:
					if (n->enable_debug) fprintf(stdout,"WARNING: Reloc fixup type %u not yet supported\n",tmp[0]);
					if (n->must_resolve_dependencies) return 1;
					break;
			}
		}
	}

	af->internal_flags |= NE_SEGMENT_ASSIGN_IF_RELOC_APPLIED;
	return 0;
}

int ne_module_load_and_apply_relocations(struct ne_module *n) {
	unsigned int x;
	int y;

	if (n == NULL) return 0;
	if (n->ne_header.segment_table_entries == 0) return 0;

	for (x=1;x <= n->ne_header.segment_table_entries;x++) {
		y = ne_module_load_and_apply_segment_relocations(n,x);
		if (y) return 1;
	}

	return 0;
}

#endif

