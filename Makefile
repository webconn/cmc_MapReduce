TARGET = cmapreduce
OBJS = main.o ui.o generic.o cmrconfig.o cmrsplit.o cmrio.o

BUILDDIR = build
SRCDIR = src
INCLUDEDIR = include

CC = gcc

CFLAGS = -std=gnu99 -I$(INCLUDEDIR) -g -Wall
LDFLAGS = -lm

OBJS := $(addprefix $(BUILDDIR)/, $(OBJS))

all: $(TARGET)

$(TARGET): $(OBJS)
	gcc -o $@ $^ $(LDFLAGS)

$(OBJS): | $(BUILDDIR)

$(BUILDDIR): 
	mkdir $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $^ $(CFLAGS)

clean:
	rm $(TARGET) $(BUILDDIR) -rf
