# Compiler settings
CC := gcc
CXX := g++
CFLAGS := -Wall -Wextra -Werror
DEV_FLAGS := -gdwarf-4 -g3
COV_FLAGS := --coverage -O0
MOCKS := MOCK_ADD

# Directories
SRC_DIR := src/
INC_DIR := src/ # inc/ in some projects
OBJ_DIR := obj/
OUT_DIR := bin/
PROD_DIR := prod/
DEV_DIR := dev/
COV_DIR := coverage/
TEST_DIR := test/
TEST_INC_DIR := test/
SITE_DIR := site/

# Paths
ROOT := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
SRC_PATH := $(ROOT)$(SRC_DIR)
INC_PATH := $(ROOT)$(INC_DIR)
TEST_SRC_PATH := $(ROOT)$(TEST_DIR)
TEST_INC_PATH := $(ROOT)$(TEST_INC_DIR)
OBJ_PATH := $(ROOT)$(OBJ_DIR)
OUT_PATH := $(ROOT)$(OUT_DIR)
OBJ_PROD_PATH := $(OBJ_PATH)$(PROD_DIR)
OBJ_DEV_PATH := $(OBJ_PATH)$(DEV_DIR)
OBJ_COV_PATH := $(OBJ_PATH)$(COV_DIR)
OBJ_TEST_PATH := $(OBJ_PATH)$(TEST_DIR)
OUT_PROD_PATH := $(OUT_PATH)$(PROD_DIR)
OUT_DEV_PATH := $(OUT_PATH)$(DEV_DIR)
OUT_TEST_PATH := $(OUT_PATH)$(TEST_DIR)
SITE_PATH := $(ROOT)$(SITE_DIR)
ifneq ($(OS),Windows_NT)
REL_PATH := $(shell realpath --relative-to=$(CURDIR) $(ROOT))/
else
REL_PATH := $(strip $(subst \,/, \
	$(shell powershell -NoProfile -Command \
		"Resolve-Path '$(ROOT)' -Relative") \
))
REL_PATH := $(if $(filter %/,$(REL_PATH)),$(REL_PATH),$(REL_PATH)/)
endif

# Defines
TEST_DEFS := $(foreach mock, $(MOCKS), -D$(mock))

# Dependencies
ifneq ($(OS),Windows_NT)
PKGS := libtorrent-rasterbar glib-2.0
LIBS := $(if $(PKGS),$(shell pkg-config --cflags --libs $(PKGS)))
endif

# Includes
INCS := -I$(INC_PATH)
TEST_INCS := -I$(TEST_INC_PATH)

# Headers
ifneq ($(OS),Windows_NT)
HEDS := $(shell find $(INC_PATH) -type f -name '*.h')
TEST_HEDS := $(shell find $(TEST_INC_PATH) -type f -name '*.h')
else
HEDS := $(subst \,/, \
	$(shell powershell -NoProfile -Command \
	"Get-ChildItem -Path $(INC_PATH) -Recurse -File -Filter *.h | \
	Select-Object -ExpandProperty FullName") \
)
TEST_HEDS := $(subst \,/, \
	$(shell powershell -NoProfile -Command \
	"Get-ChildItem -Path $(TEST_INC_PATH) -Recurse -File -Filter *.h | \
	Select-Object -ExpandProperty FullName") \
)
endif

# Sources
ifneq ($(OS),Windows_NT)
SRCS := $(shell find $(SRC_PATH) -type f \( -name '*.c' -o -name '*.cpp' \))
TEST_SRCS := $(shell find $(TEST_SRC_PATH) -type f \( -name '*.c' -o -name '*.cpp' \))
else
SRCS := $(subst \,/, \
	$(shell powershell -NoProfile -Command \
		"Get-ChildItem -Path $(SRC_PATH) -Recurse -File -Include *.c,*.cpp | \
		Select-Object -ExpandProperty FullName") \
)
TEST_SRCS := $(subst \,/, \
	$(shell powershell -NoProfile -Command \
	"Get-ChildItem -Path $(TEST_SRC_PATH) -Recurse -File -Include *.c,*.cpp | \
	Select-Object -ExpandProperty FullName") \
)
endif

# Entry points (assumes standard format)
ENT_REGEX := '^\s*int\s+main\s*\x28'
ifneq ($(OS),Windows_NT)
ENTS := $(foreach \
	src,\
	$(SRCS), \
	$(if \
		$(shell $(CXX) -E $(src) $(INCS) $(LIBS) | grep -lP $(ENT_REGEX)), \
		$(src) \
	) \
)
TEST_ENTS := $(foreach \
	src,\
	$(TEST_SRCS), \
	$(if \
		$(shell $(CXX) -E $(src) $(INCS) $(TEST_INCS) $(LIBS) | grep -lP $(ENT_REGEX)), \
		$(src) \
	) \
)
else
ENTS := $(foreach \
	src,\
	$(SRCS), \
	$(if \
		$(shell powershell -NoProfile -Command \
			"$(CXX) -E $(src) $(INCS) $(LIBS) | Select-String -Pattern $(ENT_REGEX) -CaseSensitive"), \
		$(src) \
	) \
)
TEST_ENTS := $(foreach \
	src,\
	$(TEST_SRCS), \
	$(if \
		$(shell powershell -NoProfile -Command \
			"$(CXX) -E $(src) $(INCS) $(TEST_INCS) $(LIBS) | Select-String -Pattern $(ENT_REGEX) -CaseSensitive"), \
		$(src) \
	) \
)
endif

# Objects
OBJS_PROD := $(foreach src,$(SRCS), \
	$(if $(filter %.c,$(src)),\
		$(patsubst \
			$(SRC_PATH)%.c, \
			$(OBJ_PROD_PATH)%.o, \
			$(src) \
		), \
		$(patsubst \
			$(SRC_PATH)%.cpp, \
			$(OBJ_PROD_PATH)%.o, \
			$(src) \
		) \
	) \
)
OBJS_DEV := $(foreach src,$(SRCS), \
	$(if $(filter %.c,$(src)),\
		$(patsubst \
			$(SRC_PATH)%.c, \
			$(OBJ_DEV_PATH)%.o, \
			$(src) \
		), \
		$(patsubst \
			$(SRC_PATH)%.cpp, \
			$(OBJ_DEV_PATH)%.o, \
			$(src) \
		) \
	) \
)
OBJS_TEST := $(foreach src,$(TEST_SRCS), \
	$(if $(filter %.c,$(src)),\
		$(patsubst \
			$(TEST_SRC_PATH)%.c, \
			$(OBJ_TEST_PATH)%.o, \
			$(src) \
		), \
		$(patsubst \
			$(TEST_SRC_PATH)%.cpp, \
			$(OBJ_TEST_PATH)%.o, \
			$(src) \
		) \
	) \
)

# Executables produced
OUTS_PROD := $(foreach ent,$(ENTS), \
	$(if $(filter %.c,$(ent)),\
		$(patsubst \
			$(SRC_PATH)%.c, \
			$(OUT_PROD_PATH)%, \
			$(ent) \
		), \
		$(patsubst \
			$(SRC_PATH)%.cpp, \
			$(OUT_PROD_PATH)%, \
			$(ent) \
		) \
	) \
)
OUTS_DEV := $(foreach ent,$(ENTS), \
	$(if $(filter %.c,$(ent)),\
		$(patsubst \
			$(SRC_PATH)%.c, \
			$(OUT_DEV_PATH)%, \
			$(ent) \
		), \
		$(patsubst \
			$(SRC_PATH)%.cpp, \
			$(OUT_DEV_PATH)%, \
			$(ent) \
		) \
	) \
)
OUTS_TEST := $(foreach test,$(TEST_ENTS), \
	$(if $(filter %.c,$(test)),\
		$(patsubst \
			$(TEST_SRC_PATH)%.c, \
			$(OUT_TEST_PATH)%, \
			$(test) \
		), \
		$(patsubst \
			$(TEST_SRC_PATH)%.cpp, \
			$(OUT_TEST_PATH)%, \
			$(test) \
		) \
	) \
)

# Objects without entry point
OBJS_PROD_NONENTRY := $(filter-out \
	$(foreach out, \
		$(OUTS_PROD), \
		$(patsubst \
			$(OUT_PROD_PATH)%, \
			$(OBJ_PROD_PATH)%.o, \
			$(out) \
		) \
	), \
	$(OBJS_PROD) \
)
OBJS_DEV_NONENTRY := $(filter-out \
	$(foreach out, \
		$(OUTS_DEV), \
		$(patsubst \
			$(OUT_DEV_PATH)%, \
			$(OBJ_DEV_PATH)%.o, \
			$(out) \
		) \
	), \
	$(OBJS_DEV) \
)
OBJS_COV := $(foreach obj, \
	$(OBJS_DEV_NONENTRY), \
	$(patsubst \
		$(OBJ_DEV_PATH)%.o, \
		$(OBJ_COV_PATH)%.o, \
		$(obj) \
	) \
)
OBJS_TEST_NONENTRY := $(filter-out \
	$(foreach out, \
		$(OUTS_TEST), \
		$(patsubst \
			$(OUT_TEST_PATH)%, \
			$(OBJ_TEST_PATH)%.o, \
			$(out) \
		) \
	), \
	$(OBJS_TEST) \
)

# Test run logs
TEST_LOGS = $(foreach \
	test, \
	$(OUTS_TEST), \
	$(patsubst \
		%, \
		%.log, \
		$(test) \
	) \
)

# Check for color output support
ifeq ($(NO_COLOR),)
ifneq ($(TERM),dumb)
ifneq ($(strip $(MAKE_TERMOUT)),)
COLOR_SUPPORT := true
endif
endif
endif

ifeq ($(COLOR_SUPPORT),true)
RED    := \033[1;31m
GREEN  := \033[1;32m
YELLOW := \033[1;33m
BLUE   := \033[1;34m
RESET  := \033[0m
else
RED    :=
GREEN  :=
YELLOW :=
BLUE   :=
RESET  :=
endif

ifneq ($(OS),Windows_NT)
define print-file
	@printf "make: $1\n"
endef
define print-exe
	@printf "make: $(GREEN)$1$(RESET)\n"
endef
else
define print-file
	@powershell -NoProfile -Command "Write-Host 'make: $1'"
endef
define print-exe
	@powershell -NoProfile -Command "\
		Write-Host 'make: ' -NoNewline; Write-Host '$1' -ForegroundColor Green"
endef
endif

# Function to convert absolute path inside project to relative path
define relpath
$(patsubst $(ROOT)%,$(REL_PATH)%,$1)
endef

# Function to create neccessary directories
ifneq ($(OS),Windows_NT)
define ensure-dir
	@if [ ! -d "$(dir $1)" ]; then \
		printf "make: $(BLUE)$(call relpath,$(dir $1))$(RESET)\n"; \
		mkdir -p "$(dir $1)"; \
	fi
endef
else
define ensure-dir
	@powershell -NoProfile -Command "\
		if (-not (Test-Path -Path '$(dir $1)')) { \
			Write-Host 'make: ' -NoNewline; Write-Host '$(call relpath,$(dir $1))' -ForegroundColor Blue; \
			New-Item -ItemType Directory -Force -Path '$(dir $1)' | Out-Null \
		}"
endef
endif

# Default build target
default: dev prod

# Development build target
dev: $(OUTS_DEV)

# Production build target
prod: $(OUTS_PROD)

ifneq ($(OS),Windows_NT)
# Test code with unity
test: $(TEST_LOGS)
	@PASS_COUNT=$$(grep -s ":PASS" $(TEST_LOGS) | wc -l); \
	FAIL_COUNT=$$(grep -s ":FAIL" $(TEST_LOGS) | wc -l); \
	IGNORE_COUNT=$$(grep -s ":IGNORE" $(TEST_LOGS) | wc -l); \
	if [ $$FAIL_COUNT -eq 0 ]; then \
		printf "$(GREEN)PASS: $$PASS_COUNT, FAIL: $$FAIL_COUNT, IGNORE: $$IGNORE_COUNT$(RESET)\n"; \
	else \
		printf "$(RED)PASS: $$PASS_COUNT, FAIL: $$FAIL_COUNT, IGNORE: $$IGNORE_COUNT\n\n"; \
		grep -sh :FAIL $(TEST_LOGS) || true; \
		printf "$(RESET)\n"; \
		exit 1; \
	fi

# Coverage report
report: $(SITE_PATH)index.html

# Linting check
lint:
	cpplint --filter=-legal/copyright --root=$(SRC_PATH) $(SRCS)
	cpplint --filter=-legal/copyright --root=$(INC_PATH) $(HEDS)
	cpplint --filter=-legal/copyright --root=$(TEST_SRC_PATH) $(TEST_SRCS)
	cpplint --filter=-legal/copyright --root=$(TEST_INC_DIR) $(TEST_HEDS)
	clang-tidy -header-filter='src/.*' --warnings-as-errors=* $(SRCS) -- $(INCS) $(LIBS)

# Format code
format:
	@printf "make: $(YELLOW)WARNING: 'make format' is DEPRECATED.$(RESET)\n" >&2
	clang-format -i -style=file $(SRCS) $(HEDS)
endif

# Remove generated files
clean:
ifneq ($(OS),Windows_NT)
	@rm -rf \
		$(OUTS_PROD) $(OBJS_PROD) $(OUTS_DEV) \
		$(OBJS_DEV) $(OBJ_PATH) $(OUT_PATH) $(SITE_PATH)
else
	@powershell -NoProfile -Command \
		"Remove-Item -Recurse -Force -ErrorAction SilentlyContinue \
		$(OBJ_PATH),$(OUT_PATH),$(SITE_PATH)"; \
		exit 0;
endif

# Production - Link objects into executablea
$(OUT_PROD_PATH)%: $(OBJ_PROD_PATH)%.o $(OBJS_PROD_NONENTRY)
	$(call ensure-dir,$@)
	$(call print-exe,$(call relpath,$@))
	@$(CXX) -o $@ $< $(OBJS_PROD_NONENTRY) $(CFLAGS) $(INCS) $(LIBS)

# Production - Compile C source files into object files
$(OBJ_PROD_PATH)%.o: $(SRC_PATH)%.c $(HEDS)
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@$(CC) -c -o $@ $< $(CFLAGS) $(INCS) $(LIBS)

# Production - Compile C source files into object files
$(OBJ_PROD_PATH)%.o: $(SRC_PATH)%.cpp $(HEDS)
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@$(CXX) -c -o $@ $< $(CFLAGS) $(INCS) $(LIBS)

# Development - Link objects into executablea
$(OUT_DEV_PATH)%: $(OBJ_DEV_PATH)%.o $(OBJS_DEV_NONENTRY)
	$(call ensure-dir,$@)
	$(call print-exe,$(call relpath,$@))
	@$(CXX) -o $@ $< $(OBJS_DEV_NONENTRY) $(CFLAGS) $(DEV_FLAGS) $(INCS) $(LIBS)

# Development - Compile C source files into object files
$(OBJ_DEV_PATH)%.o: $(SRC_PATH)%.c $(HEDS)
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@$(CC) -c -o $@ $< $(CFLAGS) $(DEV_FLAGS) $(INCS) $(LIBS)

# Development - Compile CPP source files into object files
$(OBJ_DEV_PATH)%.o: $(SRC_PATH)%.cpp $(HEDS)
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@$(CXX) -c -o $@ $< $(CFLAGS) $(DEV_FLAGS) $(INCS) $(LIBS)

# Coverage - Compile C source files into object files
$(OBJ_COV_PATH)%.o: $(SRC_PATH)%.c $(HEDS)
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@$(CC) -c -o $@ $< $(CFLAGS) $(COV_FLAGS) $(TEST_DEFS) $(INCS) $(LIBS)

# Coverage - Compile CPP source files into object files
$(OBJ_COV_PATH)%.o: $(SRC_PATH)%.cpp $(HEDS)
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@$(CXX) -c -o $@ $< $(CFLAGS) $(COV_FLAGS) $(TEST_DEFS) $(INCS) $(LIBS)

# Test - Link objects into executablea
$(OUT_TEST_PATH)%: $(OBJ_TEST_PATH)%.o $(OBJS_COV) $(OBJS_TEST_NONENTRY)
	$(call ensure-dir,$@)
	$(call print-exe,$(call relpath,$@))
	@$(CXX) -o $@ $< $(OBJS_COV) $(OBJS_TEST_NONENTRY) $(CFLAGS) $(COV_FLAGS) $(INCS) $(TEST_INCS) $(TEST_DEFS) $(LIBS)

# Test - Compile C test files into object files
$(OBJ_TEST_PATH)%.o: $(TEST_SRC_PATH)%.c $(HEDS) $(TEST_HEDS)
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@$(CC) -c -o $@ $< $(CFLAGS) $(INCS) $(TEST_INCS) $(TEST_DEFS) $(LIBS)

# Test - Compile CPP source files into object files
$(OBJ_DEV_PATH)%.o: $(SRC_PATH)%.cpp $(HEDS)
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@$(CXX) -c -o $@ $< $(CFLAGS) $(DEV_FLAGS) $(INCS) $(LIBS)

# Test - Run tests to generate logs
$(OUT_TEST_PATH)%.log: $(OUT_TEST_PATH)%
	$(call ensure-dir,$@)
	$(call print-file,$(call relpath,$@))
	@timeout 5 valgrind -q --leak-check=full --track-origins=yes --error-exitcode=117 $< > $@ 2>&1; \
	EXIT_CODE=$$?; \
	if [ $$EXIT_CODE -eq 117 ]; then \
		printf "$(RED)Valgrind detected memory errors. Please review $@ for more information$(RESET)\n"; \
	elif [ $$EXIT_CODE -eq 124 ]; then \
		printf "$(RED)Test timed out after 5s, check for infite loop: $<.$(RESET)\n"; \
	elif [ $$EXIT_CODE -ne 0 ] && [ $$EXIT_CODE -ne 1 ]; then \
		printf "$(RED)Test failed with exit code $$EXIT_CODE: $<$(RESET)\n"; \
	fi

# Report - Generate report
$(SITE_PATH)index.html: $(TEST_LOGS)
	$(call ensure-dir,$(SITE_PATH))
	$(call print-file,$(call relpath,$@))
	@gcovr \
		--root $(ROOT) \
		--sort uncovered-percent \
		--html --html-nested --html-theme github.dark-green \
		--html-syntax-highlighting --output $(SITE_PATH)/index.html \
		--exclude-throw-branches

.PHONY: clean default dev prod test report lint format

.PRECIOUS: $(OBJS_PROD) $(OBJS_DEV) $(OBJS_TEST) $(OUTS_TEST) $(OBJS_COV)
