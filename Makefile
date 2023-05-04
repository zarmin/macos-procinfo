CC = cc
CFLAGS = -Wall -O2
LDFLAGS =
SRCDIR = src
OBJDIR = bin
TARGET = procinfo
TARGET_SAN = procinfo_san

SOURCES = lib.c procinfo.c
OBJECTS = $(addprefix $(OBJDIR)/,$(SOURCES:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

san: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
san: LDFLAGS += -fsanitize=address,undefined
san: $(TARGET_SAN)

$(TARGET_SAN): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^	

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TARGET_SAN)