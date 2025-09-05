# Variables
BUILD_DIR = build
BIN_DIR = bin
PROJECT_NAME = sample
CMAKE_BUILD_TYPE ?= Release

# Default target
.PHONY: all
all: build

# Build the project
.PHONY: build
build:
	@echo "Building $(PROJECT_NAME)..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) ..
	@cd $(BUILD_DIR) && cmake --build . --config $(CMAKE_BUILD_TYPE)
	@echo "Build completed. Executable: $(BIN_DIR)/$(PROJECT_NAME)"

# Clean build directory
.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
	@echo "Clean completed."

# Format code using clang-format
.PHONY: format
format:
	@echo "Formatting code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i -style=file src/*.cpp include/*.hpp; \
		echo "Code formatted successfully."; \
	else \
		echo "clang-format not found. Please install it to format code."; \
	fi

# Check code formatting
.PHONY: check-format
check-format:
	@echo "Checking code formatting..."
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -style=file -dry-run -Werror src/*.cpp include/*.hpp; \
		echo "Code formatting is correct."; \
	else \
		echo "clang-format not found. Please install it to check formatting."; \
	fi

# Run the executable
.PHONY: run
run: build
	@echo "Running $(PROJECT_NAME)..."
	@./$(BUILD_DIR)/$(BIN_DIR)/$(PROJECT_NAME)


# Debug build
.PHONY: debug
debug:
	@$(MAKE) build CMAKE_BUILD_TYPE=Debug

# Release build
.PHONY: release
release:
	@$(MAKE) build CMAKE_BUILD_TYPE=Release

# Install dependencies (Ubuntu/Debian)
.PHONY: install-deps
install-deps:
	@echo "Installing dependencies..."
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install -y \
			build-essential \
			cmake \
			git \
			libpulse-dev \
			libasound2-dev \
			libjack-dev \
			clang-format; \
		echo "Dependencies installed successfully."; \
	elif command -v brew >/dev/null 2>&1; then \
		brew install cmake git clang-format; \
		echo "Dependencies installed successfully."; \
	else \
		echo "Package manager not found. Please install dependencies manually."; \
	fi

# Show help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all          - Build the project (default)"
	@echo "  build        - Build the project"
	@echo "  clean        - Clean build directory"
	@echo "  format       - Format code using clang-format"
	@echo "  check-format - Check code formatting"
	@echo "  run          - Build and run the executable"
	@echo "  debug        - Build in debug mode"
	@echo "  release      - Build in release mode"
	@echo "  install-deps - Install system dependencies"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CMAKE_BUILD_TYPE - Build type (Debug, Release, MinSizeRel, RelWithDebInfo)"
	@echo "                     Default: Release"
