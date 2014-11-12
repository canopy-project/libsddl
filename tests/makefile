all:
SOURCE_FILES := \
        test_sddl.c

TARGET := build/test_sddl

default: all

run: $(TARGET)
	LD_LIBRARY_PATH=../:../../$(CANOPY_EMBEDDED_ROOT)/build/_out/lib $(TARGET)

dbg: $(TARGET)
	LD_LIBRARY_PATH=../:../../$(CANOPY_EMBEDDED_ROOT)/build/_out/lib gdb $(TARGET)

clean:
	rm -rf build


$(TARGET) : $(SOURCE_FILES)
	mkdir -p build
	gcc -I../../3rdparty/libred/include -I../include $(SOURCE_FILES) -L../ -L../../$(CANOPY_EMBEDDED_ROOT)/build/_out/lib -lred-canopy -lsddl -lcurl -lwebsockets -lm -Wall -Werror -g -o $(TARGET)

all: $(TARGET)

