######## Build options ########

verbose = 0
CC = gcc-11

######## Build setup ########

# SRCROOT should always be the current directory
SRCROOT         = $(CURDIR)

# .o directory
ODIR            = obj

# Source VPATHS
VPATH           += $(SRCROOT)/Source
VPATH	        += $(SRCROOT)/Source/portable/MemMang
VPATH	        += $(SRCROOT)/Source/portable/GCC/POSIX
VPATH			+= $(SRCROOT)/Project

# FreeRTOS核心对象
C_FILES			+= list.c
C_FILES			+= queue.c
C_FILES			+= tasks.c
C_FILES			+= timers.c
C_FILES			+= event_groups.c

# 可移植层对象
C_FILES			+= heap_3.c
C_FILES			+= port.c

# 项目对象
C_FILES			+= main.c
C_FILES			+= DeadlockDemo.c

# 包含路径
INCLUDES        += -I$(SRCROOT)/Source/include
INCLUDES        += -I$(SRCROOT)/Source/portable/GCC/POSIX/
INCLUDES        += -I$(SRCROOT)/Project

# 生成目标文件名
OBJS = $(patsubst %.c,%.o,$(C_FILES))

######## C Flags ########

# 警告选项
CWARNS += -W
CWARNS += -Wall
CWARNS += -Wno-unused-parameter
CWARNS += -Wno-unused-variable

CFLAGS += -m32
CFLAGS += -DDEBUG=1
CFLAGS += -g -UUSE_STDIO -D__GCC_POSIX__=1
CFLAGS += -pthread
CFLAGS += -DMAX_NUMBER_OF_TASKS=10
CFLAGS += $(INCLUDES) $(CWARNS) -O2

# 链接标志
LINKFLAGS += -m32

######## Makefile targets ########

# 规则
.PHONY : all
all: FreeRTOS-DeadlockDemo


# 将.o文件放在ODIR目录中
_OBJS = $(patsubst %,$(ODIR)/%,$(OBJS))

$(ODIR)/%.o: %.c
	@mkdir -p $(dir $@)
ifeq ($(verbose),1)
	@echo ">> Compiling $<"
	$(CC) $(CFLAGS) -c -o $@ $<
else
	@echo ">> Compiling $(notdir $<)"
	@$(CC) $(CFLAGS) -c -o $@ $<
endif

FreeRTOS-DeadlockDemo: $(_OBJS)
	@echo ">> Linking $@..."
	@$(CC) $(CFLAGS) $^ $(LINKFLAGS) $(LIBS) -o $@
	@echo "-------------------------"
	@echo "BUILD COMPLETE: $@"
	@echo "-------------------------"

.PHONY : clean
clean:
	@-rm -rf $(ODIR) FreeRTOS-DeadlockDemo
	@echo "--------------"
	@echo "CLEAN COMPLETE"
	@echo "--------------"

.PHONY : run
run: FreeRTOS-DeadlockDemo
	@echo "Running Deadlock Demo..."
	@./FreeRTOS-DeadlockDemo
