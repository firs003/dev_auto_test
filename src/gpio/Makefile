#include test/Makefile

VERBOSE=@

CROSS_COMPILER_PRE=arm-linux-gnueabihf-
CC=gcc
CFLAGS=-Wall -Werror
CFLAGS += -I../../inc
LD=ld
# LDFLAGS=-L./out/ -static -llog_proxy
# LDFLAGS+=
AR=ar
STATIC_FLAGS=rc
SHARED_FLAGS=-fPIC -shared

SHARE_DIR=/home/sleng/win/share
OUT_DIR=./out

BIN_TARGET=gpio_test
BIN_SRCS=$(wildcard *.c)
BIN_OBJS=$(BIN_SRCS:%.c=%.o)
# LIB_TARGET=$(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET)
# STATIC_LIB_TARGET=$(OUT_DIR)/.a
# SHARED_LIB_TARGET=$(OUT_DIR)/.so
# LIB_SRCS=
# LIB_OBJS=$(LIB_SRCS:%.c=%.o)
# CLIENT_TARGET=
# CLIENT_OBJS=
# BIN_FILE=

all:$(BIN_TARGET) $(LIB_TARGET) $(CLIENT_TARGET) $(INPUT_TARGET) $(OUTPUT_TARGET)
lib:$(LIB_TARGET)


$(BIN_TARGET):$(BIN_OBJS) $(STATIC_LIB_TARGET) #$(SHARED_LIB_TARGET)
	@echo Generic $@ ...
	$(VERBOSE) $(CROSS_COMPILER_PRE)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	@echo Copy $@ to share_dir
	$(VERBOSE) cp $@ $(SHARE_DIR)
# $(VERBOSE) $(CROSS_COMPILER_PRE)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(BIN_OBJS)

$(BIN_OBJS):%.o:%.c
	@echo Compiling $@ from $< ...
	$(VERBOSE) $(CROSS_COMPILER_PRE)$(CC) $(CFLAGS) -o $@ -c $<

$(STATIC_LIB_TARGET):$(LIB_OBJS)
	@echo Generic static lib $@ ...
	@mkdir -p $(OUT_DIR)
	$(VERBOSE) $(CROSS_COMPILER_PRE)$(AR) $(STATIC_FLAGS) $@ $^
	@echo Copy $@ to share_dir
	$(VERBOSE) cp $@ $(SHARE_DIR)

$(SHARED_LIB_TARGET):$(LIB_OBJS)
	@echo Generic shared lib $@ ...
	@mkdir -p $(OUT_DIR)
	$(VERBOSE) $(CROSS_COMPILER_PRE)$(CC) $(SHARED_FLAGS) $^ -o $@
	@echo Copy $@ to share_dir
	$(VERBOSE) cp $@ $(SHARE_DIR)

$(LIB_OBJS):%.o:%.c
	@echo Compiling $@ from $< ...
	$(VERBOSE) $(CROSS_COMPILER_PRE)$(CC) $(CFLAGS) -o $@ -c $<

.PHONY:clean
clean:
	rm -rf $(BIN_TARGET) $(BIN_OBJS) $(LIB_TARGET) $(LIB_OBJS) $(CLIENT_TARGET) $(CLIENT_OBJS) $(INPUT_TARGET) $(OUTPUT_TARGET)
