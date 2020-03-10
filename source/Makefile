COMPONENTS := libstratosphere AppletCompanion Sysmodule
TOPTARGETS := all clean mrproper

.PHONY: $(TOPTARGETS) $(COMPONENTS)

all: $(COMPONENTS)

libstratosphere:
	$(MAKE) -C $@

AppletCompanion:
	$(MAKE) -C $@

Sysmodule: libstratosphere
	$(MAKE) -C $@

clean:
	$(MAKE) -C AppletCompanion clean
	$(MAKE) -C Sysmodule clean
	
mrproper: clean
	$(MAKE) -C libstratosphere clean