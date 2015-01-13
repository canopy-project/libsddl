# Copyright 2014-2015 SimpleThings, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# This is the makefile for libsddl.
#
LIBRED_DIR := $(CANOPY_EMBEDDED_ROOT)/3rdparty/libred
CANOPY_EDK_BUILD_NAME ?= default
CANOPY_EDK_BUILD_OUTDIR ?= _out/$(CANOPY_EDK_BUILD_NAME)

INCLUDE_FLAGS := \
                -Isrc \
                -Iinclude \
                -I$(LIBRED_DIR)/include \
                -I$(LIBRED_DIR)/under_construction

SOURCE_FILES = \
    src/sddl.c

.PHONY: default
default:
	mkdir -p $(CANOPY_EDK_BUILD_OUTDIR)
	$(CC) -fPIC -rdynamic -shared $(INCLUDE_FLAGS) $(SOURCE_FILES) $(CANOPY_CFLAGS) -o $(CANOPY_EDK_BUILD_OUTDIR)/libsddl.so

.PHONY: clean
clean:
	rm -rf $(CANOPY_EDK_BUILD_OUTDIR)

.PHONY: install
install:
	mkdir $(DESTDIR)$(PREFIX)/lib
	cp libsddl.so $(DESTDIR)$(PREFIX)/lib
    
