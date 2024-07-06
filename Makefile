# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -pthread
LDFLAGS := -lssl -lcrypto

# Source files and object files
SRCS := src/download_cert.c src/read_file.c src/get_certificate.c src/save_certificate.c src/utils.c
OBJS := $(SRCS:.c=.o)

# Output binary name
TARGET := download_cert

# Phony targets (targets that don't represent files)
.PHONY: all clean full help

# Default target
all: $(TARGET)

# Full compilation target (including libraries)
full: CFLAGS += -DFULL_COMPILE
full: LDFLAGS += -static
full: $(TARGET)

# Linking the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compiling source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cleaning up compiled files
clean:
	rm -f $(TARGET) $(OBJS) $(OBJS:.o=.d)

# Include dependencies
-include $(OBJS:.o=.d)

# Generate dependency files
%.d: %.c
	@$(CC) $(CFLAGS) -MM $< > $@

# Help target
help:
	@echo "Available targets:"
	@echo "  all       : Build the default target ($(TARGET))"
	@echo "  full      : Build with all libraries statically linked"
	@echo "  clean     : Remove all built and intermediate files"
	@echo "  help      : Display this help message"
	@echo ""
	@echo "Compiler flags:"
	@echo "  CFLAGS    : $(CFLAGS)"
	@echo "  LDFLAGS   : $(LDFLAGS)"
	@echo ""
	@echo "To use a specific compiler, set CC. For example:"
	@echo "  make CC=clang"
	@echo ""
	@echo "To add extra CFLAGS or LDFLAGS, use '+=' For example:"
	@echo "  make CFLAGS+=-DDEBUG LDFLAGS+=-L/usr/local/lib"

# Make 'help' the default target if no target is specified
.DEFAULT_GOAL := help
