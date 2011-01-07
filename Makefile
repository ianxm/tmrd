CFLAGS = -g -Wall

CC = gcc
LIBS =  -ljack -lpthread -ldl -lrt -lncurses 
INCLUDES =
OBJDIR = build
SRCDIR = src
OBJS = $(OBJDIR)/tmrd.o
SRCS = $(SRCDIR)/tmrd.c
TARGET = tmrd

all: builddir $(TARGET)

builddir:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(OBJS) $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -rf $(OBJDIR)
	rm $(TARGET)
