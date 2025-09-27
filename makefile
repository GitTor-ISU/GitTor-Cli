# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -Werror
DEV_FLAGS := -gdwarf-4 -g3
COV_FLAGS := --coverage -O0
MOCKS := MOCK_ADD

# Directories
SRC_DIR := src/
INC_DIR := src/ # inc/ in some projects
TEST_DIR := test/
TEST_INC_DIR := test/
OBJ_DIR := obj/
OUT_DIR := bin/
PROD_DIR := prod/
DEV_DIR := dev/
COV_DIR := coverage/
SITE_DIR := site/

# Paths
ROOT := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
REL_PATH := $(shell realpath --relative-to=$(CURDIR) $(ROOT))/
SRC_PATH := $(ROOT)$(SRC_DIR)
INC_PATH := $(ROOT)$(INC_DIR)
TEST_PATH := $(ROOT)$(TEST_DIR)
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

# Defines
TEST_DEFS := $(foreach mock, $(MOCKS), -D$(mock))

# Dependencies
PKGS :=
LIBS := $(if $(PKGS),$(shell pkg-config --cflags --libs $(PKGS)))

# Includes
INCS := -I$(INC_PATH)
TEST_INCS := -I$(TEST_INC_PATH)

# Headers
HEDS := $(shell find $(INC_PATH) -type f -name '*.h')
TEST_HEDS := $(shell find $(TEST_INC_PATH) -type f -name '*.h')

# Sources
SRCS := $(shell find $(SRC_PATH) -type f -name '*.c')
TEST_SRCS := $(shell find $(TEST_PATH) -type f -name '*.c')

# Entry points (assumes standard format)
ENT_REGEX := '^\s*int\s+main\s*\x28'
ENTS := $(foreach \
	src,\
	$(SRCS), \
	$(if \
		$(shell cpp -P $(src) $(INCS) $(LIBS) | grep -lP $(ENT_REGEX)), \
		$(src) \
	) \
)
TESTS := $(foreach \
	src,\
	$(TEST_SRCS), \
	$(if \
		$(shell cpp -P $(src) $(INCS) $(TEST_INCS) $(LIBS) | grep -lP $(ENT_REGEX)), \
		$(src) \
	) \
)

# Objects
OBJS_PROD := $(foreach \
	src, \
	$(SRCS), \
	$(patsubst \
		$(SRC_PATH)%.c, \
		$(OBJ_PROD_PATH)%.o, \
		$(src) \
	) \
)
OBJS_DEV := $(foreach \
	src, \
	$(SRCS), \
	$(patsubst \
		$(SRC_PATH)%.c, \
		$(OBJ_DEV_PATH)%.o, \
		$(src) \
	) \
)
OBJS_TEST := $(foreach \
	src, \
	$(TEST_SRCS), \
	$(patsubst \
		$(TEST_PATH)%.c, \
		$(OBJ_TEST_PATH)%.o, \
		$(src) \
	) \
)

# Executables produced
OUTS_PROD := $(foreach \
	ent, \
	$(ENTS), \
	$(patsubst \
		$(SRC_PATH)%.c, \
		$(OUT_PROD_PATH)%, \
		$(ent) \
	) \
)
OUTS_DEV := $(foreach \
	ent, \
	$(ENTS), \
	$(patsubst \
		$(SRC_PATH)%.c, \
		$(OUT_DEV_PATH)%, \
		$(ent) \
	) \
)
OUTS_TEST := $(foreach \
	test, \
	$(TESTS), \
	$(patsubst \
		$(TEST_PATH)%.c, \
		$(OUT_TEST_PATH)%, \
		$(test) \
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

# Function to convert absolute path inside project to relative path
define relpath
$(patsubst $(ROOT)%,$(REL_PATH)%,$1)
endef

# Function to create neccessary directories
define ensure-dir
	@if [ ! -d "$(dir $1)" ]; then \
		mkdir -p "$(dir $1)"; \
		printf "make: \033[1;34m$(call relpath,$(dir $1))\033[0m\n"; \
	fi
endef

# Default build target
default: dev prod

# Development build target
dev: $(OUTS_DEV)

# Production build target
prod: $(OUTS_PROD)

# Test code with unity
test: $(TEST_LOGS)
	@PASS_COUNT=$$(grep -s ":PASS" $(TEST_LOGS) | wc -l); \
	FAIL_COUNT=$$(grep -s ":FAIL" $(TEST_LOGS) | wc -l); \
	IGNORE_COUNT=$$(grep -s ":IGNORE" $(TEST_LOGS) | wc -l); \
	if [ $$FAIL_COUNT -eq 0 ]; then \
		printf "\033[;32mPASS: $$PASS_COUNT, FAIL: $$FAIL_COUNT, IGNORE: $$IGNORE_COUNT\033[;0m\n"; \
	else \
		printf "\033[;31mPASS: $$PASS_COUNT, FAIL: $$FAIL_COUNT, IGNORE: $$IGNORE_COUNT\n\n"; \
		grep -sh :FAIL $(TEST_LOGS) || true; \
		printf "\033[;0m\n"; \
		exit 1; \
	fi

# Coverage report
report: $(SITE_PATH)index.html

# Linting check
lint:
	cpplint --filter=-legal/copyright --root=$(SRC_PATH) $(SRCS)
	cpplint --filter=-legal/copyright --root=$(INC_PATH) $(HEDS)
	cpplint --filter=-legal/copyright --root=$(TEST_PATH) $(TEST_SRCS)
	cpplint --filter=-legal/copyright --root=$(TEST_INC_DIR) $(TEST_HEDS)
	clang-tidy -header-filter='src/.*' --warnings-as-errors=* $(SRCS) -- $(INCS) $(LIBS)

# Format code
format:
	@printf "make: \033[1;33mWARNING: 'make format' is DEPRECATED.\033[0m\n" >&2
	clang-format -i -style=file -assume-filename=.c $(SRCS) $(HEDS)

# Remove generated files
clean:
	@rm -rf \
		$(OUTS_PROD) $(OBJS_PROD) $(OUTS_DEV) \
		$(OBJS_DEV) $(OBJ_PATH) $(OUT_PATH) $(SITE_PATH)

# Production - Link objects into executablea
$(OUT_PROD_PATH)%: $(OBJ_PROD_PATH)%.o $(OBJS_PROD_NONENTRY)
	$(call ensure-dir,$@)
	@printf "make: \033[1;32m$(call relpath,$@)\033[0m\n"
	@$(CC) -o $@ $< $(OBJS_PROD_NONENTRY) $(CFLAGS) $(INCS) $(LIBS)

# Production - Compile C source files into object files
$(OBJ_PROD_PATH)%.o: $(SRC_PATH)%.c $(HEDS)
	$(call ensure-dir,$@)
	@printf "make: $(call relpath,$@)\n"
	@$(CC) -c -o $@ $< $(CFLAGS) $(INCS) $(LIBS)

# Development - Link objects into executablea
$(OUT_DEV_PATH)%: $(OBJ_DEV_PATH)%.o $(OBJS_DEV_NONENTRY)
	$(call ensure-dir,$@)
	@printf "make: \033[1;32m$(call relpath,$@)\033[0m\n"
	@$(CC) -o $@ $< $(OBJS_DEV_NONENTRY) $(CFLAGS) $(DEV_FLAGS) $(INCS) $(LIBS)

# Development - Compile C source files into object files
$(OBJ_DEV_PATH)%.o: $(SRC_PATH)%.c $(HEDS)
	$(call ensure-dir,$@)
	@printf "make: $(call relpath,$@)\n"
	@$(CC) -c -o $@ $< $(CFLAGS) $(DEV_FLAGS) $(INCS) $(LIBS)

# Coverage - Compile C source files into object files
$(OBJ_COV_PATH)%.o: $(SRC_PATH)%.c $(HEDS)
	$(call ensure-dir,$@)
	@printf "make: $(call relpath,$@)\n"
	@$(CC) -c -o $@ $< $(CFLAGS) $(COV_FLAGS) $(TEST_DEFS) $(INCS) $(LIBS)

# Test - Link objects into executablea
$(OUT_TEST_PATH)%: $(OBJ_TEST_PATH)%.o $(OBJS_COV) $(OBJS_TEST_NONENTRY)
	$(call ensure-dir,$@)
	@printf "make: \033[1;32m$(call relpath,$@)\033[0m\n"
	@$(CC) -o $@ $< $(OBJS_COV) $(OBJS_TEST_NONENTRY) $(CFLAGS) $(COV_FLAGS) $(INCS) $(TEST_INCS) $(TEST_DEFS) $(LIBS)

# Test - Compile C test files into object files
$(OBJ_TEST_PATH)%.o: $(TEST_PATH)%.c $(HEDS) $(TEST_HEDS)
	$(call ensure-dir,$@)
	@printf "make: $(call relpath,$@)\n"
	@$(CC) -c -o $@ $< $(CFLAGS) $(INCS) $(TEST_INCS) $(TEST_DEFS) $(LIBS)

# Test - Run tests to generate logs
$(OUT_TEST_PATH)%.log: $(OUT_TEST_PATH)%
	@timeout 5 valgrind -q --leak-check=full --track-origins=yes --error-exitcode=117 $< > $@ 2>&1; \
	EXIT_CODE=$$?; \
	if [ $$EXIT_CODE -eq 117 ]; then \
		printf "\033[;31mValgrind detected memory errors. Please review $@ for more information\033[;0m\n"; \
	elif [ $$EXIT_CODE -eq 124 ]; then \
		printf "\033[;33mTest timed out after 5s, check for infite loop: $<.\033[;0m\n"; \
	elif [ $$EXIT_CODE -ne 0 ] && [ $$EXIT_CODE -ne 1 ]; then \
		printf "\033[;31mTest failed with exit code $$EXIT_CODE: $<\033[;0m\n"; \
	fi

# Report - Generate report
$(SITE_PATH)index.html: $(TEST_LOGS)
	$(call ensure-dir,$(SITE_PATH))
	@gcovr \
		--root $(ROOT) \
		--sort uncovered-percent \
		--html --html-nested --html-theme github.dark-green \
		--html-syntax-highlighting --output $(SITE_PATH)/index.html

.PHONY: clean default dev prod test report lint format

.PRECIOUS: $(OBJS_PROD) $(OBJS_DEV) $(OBJS_TEST) $(OUTS_TEST) $(OBJS_COV)
