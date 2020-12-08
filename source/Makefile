COMPONENTS := libstratosphere AppletCompanion Sysmodule
TOPTARGETS := all clean mrproper

.PHONY: $(TOPTARGETS) $(COMPONENTS)

all: $(COMPONENTS)

libstratosphere:
	@echo Building libstratosphere...
	@$(MAKE) --no-print-directory -C $@

AppletCompanion:
	@echo Building applet companion...
	@$(MAKE) --no-print-directory -C $@

Sysmodule: libstratosphere
	@echo Building sysmodule...
	@$(MAKE) --no-print-directory -C $@

clean:
	@echo Cleaning applet companion...
	@$(MAKE) --no-print-directory -C AppletCompanion clean
	@echo Cleaning sysmodule...
	@$(MAKE) --no-print-directory -C Sysmodule clean
	
mrproper: clean
	@echo Cleaning libstratosphere...
	@$(MAKE) --no-print-directory -C libstratosphere clean