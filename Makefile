CC       ?= gcc

TARGET   := muterm
SRCDIR   := .
BUILDDIR := build

CFLAGS   ?= -O2 -pipe -Wall -Wextra -std=c11
LDLIBS   ?= -lSDL2 -lSDL2_ttf -lSDL2_image

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(BUILDDIR) $(TARGET)
