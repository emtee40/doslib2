
BUILD_dos16r=1
BUILD_dos16r_mm=compact

-include ../../config.mak
-include Makefile.wcc

debugme:
	@echo Enabled CPU: $(BUILD_enabled_cpus)
	@echo Enabled MM: $(BUILD_enabled_mm)
	@echo Targets: $(BUILD_targets)
	@echo suffixes "$(suffixes)"

all:
	+@for i in "" $(BUILD_targets); do\
		if [ x"$$i" != x ]; then \
			$(MAKE) --no-print-directory -$(MAKEFLAGS) all-targets target_subdir=$$i || break;\
		fi;\
	done

clean:
	@rm -f *~
	+@for i in "" $(BUILD_targets); do\
		if [ x"$$i" != x ]; then \
			$(MAKE) --no-print-directory -$(MAKEFLAGS) clean-targets target_subdir=$$i || break;\
		fi;\
	done

disk:
	dd if=/dev/zero of=test.dsk bs=512 count=2880
	mkfs.vfat test.dsk
	mcopy -i test.dsk ../../build/windows/asmhello/win16r/10_086s/exe/hello.exe ::r10_086s.exe
	mcopy -i test.dsk ../../build/windows/asmhello/win16r/20_086s/exe/hello.exe ::r20_086s.exe
	mcopy -i test.dsk ../../build/windows/asmhello/win16r/30_086s/exe/hello.exe ::r30_086s.exe
