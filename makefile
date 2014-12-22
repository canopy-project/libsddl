#CFLAGS := --std=c89 --pedantic -Wall -Werror
CFLAGS := -Wall -Werror
DEBUG_FLAGS := $(CFLAGS) -g
RELEASE_FLAGS := $(CFLAGS) -O3

LIBRED_DIR := $(CANOPY_EMBEDDED_ROOT)/3rdparty/libred

INCLUDE_FLAGS := -Isrc -Iinclude -I$(LIBRED_DIR)/include -I$(LIBRED_DIR)/under_construction

SOURCE_FILES = \
    src/sddl.c

debug:
	gcc -fPIC -rdynamic -shared $(INCLUDE_FLAGS) $(SOURCE_FILES) $(DEBUG_FLAGS) -o libsddl.so

release:
	gcc -fPIC -rdynamic -shared $(INCLUDE_FLAGS) $(SOURCE_FILES) $(RELEASE_FLAGS) -o libsddl.so

clean:
	rm -f libsddl.so

install:
	mkdir $(DESTDIR)$(PREFIX)/lib
	cp libsddl.so $(DESTDIR)$(PREFIX)/lib
    
