# Filename: GNUmakefile / v2024.03.25-001, part of gcc-bison-flex-GNUmakefile

# Includes the main.mk makefile of the GNU Make framework. If couldn't be found, it shall fail
$(if $(MKFWK_MAIN_MAKEFILE),$(eval include $(MKFWK_MAIN_MAKEFILE)),$(eval include ../mkframework/main.mk))

# Prevents GNU Make from even considering to remake this very same makefile, as that isn't necessary, thus optimizing startup time
.PHONY: $(MKFWK_LAST_INCLUDING_DIR)GNUmakefile

LAST_INCLUDING_DIR-src:=$(MKFWK_LAST_INCLUDING_DIR)
include $(LAST_INCLUDING_DIR-src)utils/include.mk
include $(LAST_INCLUDING_DIR-src)cpu/include.mk
include $(LAST_INCLUDING_DIR-src)filesystem/include.mk
include $(LAST_INCLUDING_DIR-src)kernel/include.mk
include $(LAST_INCLUDING_DIR-src)memoria/include.mk

# Parses the unexpanded canned directives of the MKFWK_FOOTER variable defined by the GNU Make framework
$(eval $(value MKFWK_FOOTER))