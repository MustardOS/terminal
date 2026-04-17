# make CC=aarch64-linux-gcc TARGET=../frontend/bin/muterm

CC       ?= gcc

TARGET   := muterm
SRCDIR   := .
BUILDDIR := build

CFLAGS   ?= -O2 -pipe -Wall -Wextra -std=c11
LDLIBS   ?= -lSDL2 -lSDL2_ttf -lSDL2_image

CFLAGS   += -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -s -o $@ $^ $(LDLIBS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(BUILDDIR) $(TARGET)
