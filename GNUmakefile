# Filename: GNUmakefile / v2024.03.25-001, part of gcc-bison-flex-GNUmakefile

# Includes the main.mk makefile of the GNU Make framework. If couldn't be found, it shall fail
$(if $(MKFWK_MAIN_MAKEFILE),$(eval include $(MKFWK_MAIN_MAKEFILE)),$(eval include mkframework/main.mk))

# Prevents GNU Make from even considering to remake this very same makefile, as that isn't necessary, thus optimizing startup time
.PHONY: $(MKFWK_LAST_INCLUDING_DIR)GNUmakefile

include $(MKFWK_LAST_INCLUDING_DIR)src/include.mk

# Parses the unexpanded canned directives of the MKFWK_FOOTER variable defined by the GNU Make framework
$(eval $(value MKFWK_FOOTER))