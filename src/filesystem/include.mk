# Filename: GNUmakefile / v2024.03.25-001, part of gcc-bison-flex-GNUmakefile

# Includes the main.mk makefile of the GNU Make framework. If couldn't be found, it shall fail
$(if $(MKFWK_MAIN_MAKEFILE),$(eval include $(MKFWK_MAIN_MAKEFILE)),$(eval include ../../mkframework/main.mk))

# Prevents GNU Make from even considering to remake this very same makefile, as that isn't necessary, thus optimizing startup time
.PHONY: $(MKFWK_LAST_INCLUDING_DIR)GNUmakefile

# Basename for the program
program2:=filesystem

# Adds to the list of binary prefixes
BINARY_PREFIXES+=$(program2)_BIN
$(program2)_BINDIR:=$(MKFWK_LAST_INCLUDING_DIR)bin/

# Space-separated basenames of the programs to be made and placed into $(program2)_BIN
$(program2)_BIN_PROGRAMS+=$(program2)

# Executing the program with this makefile.
#   The arguments to pass to the program.
$(program2)_ARGS:=
#   The working directory for the program. Alternatively, it can be left empty to use the current directory.
$(program2)_CWD:=$(MKFWK_LAST_INCLUDING_DIR)

#   Subdirectory to search for source files for the program. Alternatively, it can be left empty to use the current directory. By default: src/
$(program2)_SRCDIR:=$(MKFWK_LAST_INCLUDING_DIR)src/
#   Checks that the set directory above exists
$(call mkfwk_make_check_set_directory_existence,$(program2)_SRCDIR)
#   Finds source files under the set directory above
$(program2)_FIND_SOURCES:=$(shell $(FIND) $(if $($(program2)_SRCDIR),'$($(program2)_SRCDIR)',.) -type f \( \( -name '*.c' ! -name '*.tab.c' ! -name '*.lex.yy.c' \) -o \( -name '*.y' -o -name '*.l' \) \) -print | $(SED) -e 's?^\./??' ;)
#     Note: By default, the *find* command does recursive searchs. If you want a max depth of 1, you may add: ! -path '.' -prune
#   Sets the found source files as the program's source files
$(program2)_SOURCES:=$($(program2)_FIND_SOURCES)
#   Adds the static library as a prerequisite to the program
$(program2)_SOURCES+=$($(library1)_BINDIR)libutils.a
#   Sets the program's linking order. You may add the -l and -L linking flags here
$(program2)_LDADD:=$($(program2)_SOURCES) -lutils -lcommons -lpthread -lreadline -lm

# Add here the options to be passed to CC for the preprocessing phase for $(program2)
$(program2)_CPPFLAGS=
# Add here the options to be passed to CC for $(program2)
$(program2)_CFLAGS=
# Add here the options to be passed to CC for the assembling phase for $(program2)
$(program2)_ASFLAGS=
# Add here the options to be passed to CC for the linking phase for $(program2)
$(program2)_LDFLAGS=

# Subdirectory to search for *.h header files.
#   Alternatively, it can be left empty to use the current directory. By default: $($(program2)_SRCDIR)
$(program2)_INCLUDEDIR:=$($(program2)_SRCDIR)
# Checks that the set directory above exists
$(call mkfwk_make_check_set_directory_existence,$(program2)_INCLUDEDIR)
# Finds *.h header files and produces -I'dir' options correspondingly
$(program2)_FIND_-I_FLAGS:=\
$(sort $(shell $(FIND) $(if $($(program2)_SRCDIR),'$($(program2)_INCLUDEDIR)',.) -type f -name '*.h' -print | $(SED) -e 's?^\./??' -e 's?[^/]*$$??' -e 's?/$$??' -e "s?^?-I'?" -e "s?\$$?'?" -e "s?^-I''\$$?-I'.'?" ;)) \
$(if $(filter %.y,$($(program2)_SOURCES)),$(shell $(PRINTF) '%s\n' '$(subst $(SPACE),'$(SPACE)',$(patsubst %.y,$(OBJDIR)%,$(filter %.y,$($(program2)_SOURCES))))' | $(SED) -e 's?[^/]*$$??' -e 's?/$$??' -e "s?^?-I'?" -e "s?\$$?'?" -e "s?^-I''\$$?-I'.'?" ;))
# Adds the produced -I'dir' options to be passed to CC for the preprocessing phase for $(program2)
$(program2)_CPPFLAGS+=$($(program2)_FIND_-I_FLAGS)
# Adds the prerequisite's -I'dir' options
$(program2)_CPPFLAGS+=$($(library1)_FIND_-I_FLAGS)

# Subdirectory to search for library files (lib*.a and lib*.so files).
#   Alternatively, it can be left empty to use the current directory. By default: $($(program2)_SRCDIR)
$(program2)_LIBDIR:=$($(program2)_SRCDIR)
# Checks that the set directory above exists
$(call mkfwk_make_check_set_directory_existence,$(program2)_LIBDIR)
# Finds library files and produces -L'dir' options correspondingly
$(program2)_FIND_-L_FLAGS:=\
$(sort $(shell $(FIND) $(if $($(program2)_SRCDIR),'$($(program2)_LIBDIR)',.) -type f \( -name 'lib*.a' -o -name 'lib*.so' \) -print | $(SED) -e 's?^\./??' -e 's?[^/]*$$??' -e 's?/$$??' -e "s?^?-L'?" -e "s?\$$?'?" -e "s?^-L''\$$?-L'.'?" ;))
# Adds the produced -L'dir' options to be passed to CC for the linking phase for $(program2)
$(program2)_LDFLAGS+=$($(program2)_FIND_-L_FLAGS)
# Adds the prerequired generated library
$(program2)_LDFLAGS+=-L'$(if $($(library1)_BINDIR),$($(library1)_BINDIR),.)'

# Parses the unexpanded canned directives of the MKFWK_FOOTER variable defined by the GNU Make framework
$(eval $(value MKFWK_FOOTER))