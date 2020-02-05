COMPONENTS := AppletCompanion Sysmodule
TOPTARGETS := all clean

.PHONY: $(TOPTARGETS) $(COMPONENTS)

all: $(COMPONENTS)

AppletCompanion:
	$(MAKE) -C $@ -j$(nproc)

Sysmodule:
	$(MAKE) -C $@ -j$(nproc)

clean:
	$(MAKE) -C AppletCompanion clean
	$(MAKE) -C Sysmodule clean