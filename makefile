LIBRED_DIR := $(CANOPY_EMBEDDED_ROOT)/3rdparty/libred

INCLUDE_FLAGS := -Isrc -Iinclude -I$(LIBRED_DIR)/include -I$(LIBRED_DIR)/under_construction

SOURCE_FILES = \
    src/sddl.c

.PHONY: default
default:
	$(CC) -fPIC -rdynamic -shared $(INCLUDE_FLAGS) $(SOURCE_FILES) $(CFLAGS) -o libsddl.so

.PHONY: clean
clean:
	rm -f libsddl.so

.PHONY: install
install:
	mkdir $(DESTDIR)$(PREFIX)/lib
	cp libsddl.so $(DESTDIR)$(PREFIX)/lib
    
