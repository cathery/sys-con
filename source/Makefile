COMPONENTS := AppletCompanion Sysmodule
TOPTARGETS := all clean mrproper

.PHONY: $(TOPTARGETS) $(COMPONENTS)

all: $(COMPONENTS)

AppletCompanion:
	@$(MAKE) --no-print-directory -C $@

Sysmodule:
	@$(MAKE) --no-print-directory -C $@

clean:
	@$(MAKE) --no-print-directory -C AppletCompanion clean
	@$(MAKE) --no-print-directory -C Sysmodule clean
