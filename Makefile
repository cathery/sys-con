.PHONY: all build clean

all: build
	rm -rf out
	mkdir -p out/atmosphere/contents/690000000000000D/flags
	mkdir -p out/config/sys-con
	mkdir -p out/switch/
	touch out/atmosphere/contents/690000000000000D/flags/boot2.flag
	cp source/Sysmodule/sys-con.nsp out/atmosphere/contents/690000000000000D/exefs.nsp
	cp source/AppletCompanion/sys-con.nro out/switch/sys-con.nro
	cp -r common/. out/
	@echo [DONE] sys-con compiled successfully. All files have been placed in out/

build:
	$(MAKE) -C source/

clean:
	$(MAKE) -C source/ clean
	rm -rf out