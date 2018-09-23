RM := rm -rf
MKDIR_P = mkdir -p

UNUSED := -Wno-unused-variable

OBJS += \
build/tcpdump.o \
build/termoutput.o \
build/processdata.o \
build/ptop.o \
build/util.o \
build/main.o

CPP_DEPS += \
build/tcpdump.d \
build/termoutput.d \
build/processdata.d \
build/ptop.d \
build/util.d \
build/main.d

LIBS := -lncurses -lstdc++fs -lpthread

OUT_DIR = build

# OLEVEL := -O0
# OLEVEL := -O3
OLEVEL := -Ofast -march=native -frename-registers -fno-signed-zeros -fno-trapping-math

all: directories ctop

ctop: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L/usr/lib -std=c++17 -o "./ctop" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

build/%.o: ./src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__GXX_EXPERIMENTAL_CXX0X__ -I/usr/include/jsoncpp $(OLEVEL) -g3 -Wall -c -fmessage-length=0 -std=c++17 $(UNUSED) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

clean:
	-$(RM) $(OBJS)$(C++_DEPS) ./ctop
	-@echo ' '

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}

.PHONY: all clean dependents directories
.SECONDARY:
