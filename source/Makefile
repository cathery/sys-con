COMPONENTS := AppletCompanion Sysmodule
TOPTARGETS := all clean mrproper

.PHONY: $(TOPTARGETS) $(COMPONENTS)

all: $(COMPONENTS)

AppletCompanion:
	$(MAKE) -C $@

Sysmodule:
	$(MAKE) -C $@

clean:
	$(MAKE) -C AppletCompanion clean
	$(MAKE) -C Sysmodule clean
