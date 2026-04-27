# =============================================================================
# Makefile for Chub Game Hub
#
# Cross-platform build (Linux, macOS, Windows)
# Based on the fighting-game-master Makefile architecture
#
# Usage:
#   make               - compile and run
#   make compile       - compile only
#   make run           - run the compiled binary
#   make clean         - remove build artifacts
#   make cleanAndCompile - clean then recompile
#
# For WebAssembly, use buildwasm.ps1 instead.
# =============================================================================


# -----------------------------------------------------------------------------
# OS DETECTION
# -----------------------------------------------------------------------------
UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(UNAME_S), Linux)
    DETECTED_OS := Linux
    IS_WINDOWS  := 0
else ifeq ($(UNAME_S), Darwin)
    DETECTED_OS := macOS
    IS_WINDOWS  := 0
else ifneq ($(filter MINGW% MSYS% CYGWIN%, $(UNAME_S)),)
    DETECTED_OS := Windows_MSYS2
    IS_WINDOWS  := 1
else ifeq ($(OS), Windows_NT)
    DETECTED_OS := Windows_Native
    IS_WINDOWS  := 1
else
    DETECTED_OS := Unknown
    IS_WINDOWS  := 0
endif

ifeq ($(DETECTED_OS), Windows_Native)
    SHELL       := cmd.exe
    .SHELLFLAGS := /C
endif


# -----------------------------------------------------------------------------
# EXECUTABLE NAME
# -----------------------------------------------------------------------------
TARGET_NAME := chub

ifeq ($(IS_WINDOWS), 1)
    TARGET_EXEC := $(TARGET_NAME).exe
else
    TARGET_EXEC := $(TARGET_NAME)
endif


# -----------------------------------------------------------------------------
# DIRECTORIES
# -----------------------------------------------------------------------------
BUILD_DIR := build
CORE_DIR  := core
GAMES_DIR := games


# -----------------------------------------------------------------------------
# SOURCES (recursive wildcard for portability)
# -----------------------------------------------------------------------------
rwildcard = $(foreach d,$(wildcard $(1:=/*)),\
                $(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRCS := $(call rwildcard,$(CORE_DIR),*.c) $(call rwildcard,$(GAMES_DIR),*.c)


# -----------------------------------------------------------------------------
# OBJECTS AND DEPENDENCIES
# -----------------------------------------------------------------------------
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)


# -----------------------------------------------------------------------------
# INCLUDE PATHS
#   - core/include       : game.h, hub.h, registry.h, chub_colors.h
#   - core/include/raylib : raylib.h, raymath.h, etc.
# -----------------------------------------------------------------------------
INC_DIRS := core/include core/include/raylib
INC_FLAGS := $(addprefix -I,$(INC_DIRS))


# -----------------------------------------------------------------------------
# COMPILER FLAGS
# -----------------------------------------------------------------------------
CFLAGS := $(INC_FLAGS) -MMD -MP -O1 -Wall -Wextra \
          -Wno-unused-parameter -std=c99 -Wno-missing-braces


# -----------------------------------------------------------------------------
# LINKER FLAGS
# -----------------------------------------------------------------------------
ifeq ($(DETECTED_OS), Linux)
    LDFLAGS := -Llib/linux -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
else ifeq ($(DETECTED_OS), macOS)
    LDFLAGS := -lraylib -framework OpenGL -framework Cocoa \
               -framework IOKit -framework CoreVideo -lm
else
    # Windows (MSYS2 or native)
    LDFLAGS := -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm -lm
endif


# -----------------------------------------------------------------------------
# SHELL COMMANDS (platform-dependent)
# -----------------------------------------------------------------------------
ifeq ($(DETECTED_OS), Windows_Native)
    MKDIR  = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RM_ALL = if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
    RUN    = $(subst /,\,$(BUILD_DIR)/$(TARGET_EXEC))
else
    MKDIR  = mkdir -p $(1)
    RM_ALL = rm -rf $(BUILD_DIR)
    RUN    = ./$(BUILD_DIR)/$(TARGET_EXEC)
endif


# =============================================================================
# TARGETS
# =============================================================================
.PHONY: all compile run clean cleanAndCompile

all: compile run

compile: $(BUILD_DIR)/$(TARGET_EXEC)

cleanAndCompile: clean compile

# Link
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(call MKDIR,$(BUILD_DIR))
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile C
$(BUILD_DIR)/%.c.o: %.c
	$(call MKDIR,$(@D))
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	@$(RM_ALL)

# Run
run:
	$(RUN)

# Include auto-generated dependency files
-include $(DEPS)
