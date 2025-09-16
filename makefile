# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -Werror
DEGUBFLAGS := -gdwarf-4 -g3 --coverage -O0

# Directories
SRC_DIR := src/
OBJ_DIR := obj/
OUT_DIR := bin/
PROD_DIR := prod/
DEV_DIR := dev/
TEST_DIR := test/
SITE_DIR := site/

# Dependencies
PKGS :=
LIBS := $(if $(PKGS),$(shell pkg-config --cflags --libs $(PKGS)))

# Headers
HEDS := $(shell find $(SRC_DIR) -type f -name '*.h')
INCS := -I$(SRC_DIR)
TEST_HEDS := $(HEDS) $(shell find $(TEST_DIR) -type f -name '*.h')
TEST_INCS := -I$(SRC_DIR) -I$(TEST_DIR)

# Sources
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
TESTS_SRCS := $(shell find $(TEST_DIR) -type f -name '*.c')

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
	$(TESTS_SRCS), \
	$(if \
		$(shell cpp -P $(src) $(TEST_INCS) $(LIBS) | grep -lP $(ENT_REGEX)), \
		$(src) \
	) \
)

# Objects
OBJS_PROD := $(foreach \
	src, \
	$(SRCS), \
	$(patsubst \
		$(SRC_DIR)%.c, \
		$(OBJ_DIR)$(PROD_DIR)%.o, \
		$(src) \
	) \
)
OBJS_DEV := $(foreach \
	src, \
	$(SRCS), \
	$(patsubst \
		$(SRC_DIR)%.c, \
		$(OBJ_DIR)$(DEV_DIR)%.o, \
		$(src) \
	) \
)
OBJS_TEST := $(foreach \
	src, \
	$(TESTS_SRCS), \
	$(patsubst \
		$(TEST_DIR)%.c, \
		$(OBJ_DIR)$(TEST_DIR)%.o, \
		$(src) \
	) \
)

# Executables produced
OUTS_PROD := $(foreach \
	ent, \
	$(ENTS), \
	$(patsubst \
		$(SRC_DIR)%.c, \
		$(OUT_DIR)$(PROD_DIR)%, \
		$(ent) \
	) \
)
OUTS_DEV := $(foreach \
	ent, \
	$(ENTS), \
	$(patsubst \
		$(SRC_DIR)%.c, \
		$(OUT_DIR)$(DEV_DIR)%, \
		$(ent) \
	) \
)
OUTS_TEST := $(foreach \
	test, \
	$(TESTS), \
	$(patsubst \
		$(TEST_DIR)%.c, \
		$(OUT_DIR)$(TEST_DIR)%, \
		$(test) \
	) \
)

# Objects without entry point
OBJS_NONENTRY_PROD := $(filter-out \
	$(foreach out, \
		$(OUTS_PROD), \
		$(patsubst \
			$(OUT_DIR)$(PROD_DIR)%, \
			$(OBJ_DIR)$(PROD_DIR)%.o, \
			$(out) \
		) \
	), \
	$(OBJS_PROD) \
)
OBJS_NONENTRY_DEV := $(filter-out \
	$(foreach out, \
		$(OUTS_DEV), \
		$(patsubst \
			$(OUT_DIR)$(DEV_DIR)%, \
			$(OBJ_DIR)$(DEV_DIR)%.o, \
			$(out) \
		) \
	), \
	$(OBJS_DEV) \
)
OBJS_NONENTRY_TEST := $(filter-out \
	$(foreach out, \
		$(OUTS_TEST), \
		$(patsubst \
			$(OUT_DIR)$(TEST_DIR)%, \
			$(OBJ_DIR)$(TEST_DIR)%.o, \
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

# Coverage files
GCDAS := $(foreach \
	obj, \
	$(OBJS_NONENTRY_DEV), \
	$(patsubst \
		%.o, \
		%.gcda, \
		$(obj) \
	) \
)
GCNOS := $(foreach \
	obj, \
	$(OBJS_NONENTRY_DEV), \
	$(patsubst \
		%.o, \
		%.gcda, \
		$(obj) \
	) \
)

# Function to create neccessary directories
define ensure-dir
	@if [ ! -d "$(dir $1)" ]; then \
		mkdir -p "$(dir $1)"; \
		printf "make: \033[1;34m$(dir $1)\033[0m\n"; \
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
report: $(SITE_DIR)index.html

# Format code
format:
	clang-format -i -style=file -assume-filename=.c $(SRCS) $(HEDS)

# Linting check
lint:
	cpplint --filter=-legal/copyright --root=$(SRC_DIR) $(SRCS) $(HEDS)
	clang-tidy -header-filter='src/.*' --warnings-as-errors=* $(SRCS) -- $(INCS) $(LIBS)

# Remove generated files
clean:
	@rm -rf \
		$(OUTS_PROD) $(OBJS_PROD) $(OUTS_DEV) \
		$(OBJS_DEV) $(OBJ_DIR) $(OUT_DIR) $(SITE_DIR)

# Production - Link objects into executablea
$(OUT_DIR)$(PROD_DIR)%: $(OBJ_DIR)$(PROD_DIR)%.o $(OBJS_NONENTRY_PROD)
	$(call ensure-dir,$@)
	@printf "make: \033[1;32m$@\033[0m\n"
	@$(CC) -o $@ $< $(OBJS_NONENTRY_PROD) $(CFLAGS) $(INCS) $(LIBS)

# Production -Compile C source files into object files
$(OBJ_DIR)$(PROD_DIR)%.o: $(SRC_DIR)%.c $(HEDS)
	$(call ensure-dir,$@)
	@printf "make: $@\n"
	@$(CC) -c -o $@ $< $(CFLAGS) $(INCS) $(LIBS)

# Development - Link objects into executablea
$(OUT_DIR)$(DEV_DIR)%: $(OBJ_DIR)$(DEV_DIR)%.o $(OBJS_NONENTRY_DEV)
	$(call ensure-dir,$@)
	@printf "make: \033[1;32m$@\033[0m\n"
	@$(CC) -o $@ $< $(OBJS_NONENTRY_DEV) $(CFLAGS) $(DEGUBFLAGS) $(INCS) $(LIBS)

# Development - Compile C source files into object files
$(OBJ_DIR)$(DEV_DIR)%.o: $(SRC_DIR)%.c $(HEDS)
	$(call ensure-dir,$@)
	@printf "make: $@\n"
	@$(CC) -c -o $@ $< $(CFLAGS) $(DEGUBFLAGS) $(INCS) $(LIBS)

# Test - Link objects into executablea
$(OUT_DIR)$(TEST_DIR)%: $(OBJ_DIR)$(TEST_DIR)%.o $(OBJS_NONENTRY_DEV) $(OBJS_NONENTRY_TEST)
	$(call ensure-dir,$@)
	@printf "make: \033[1;32m$@\033[0m\n"
	@$(CC) -o $@ $< $(OBJS_NONENTRY_DEV) $(OBJS_NONENTRY_TEST) $(CFLAGS) $(DEGUBFLAGS) $(TEST_INCS) $(LIBS)

# Test - Compile C test files into object files
$(OBJ_DIR)$(TEST_DIR)%.o: $(TEST_DIR)%.c $(TEST_HEDS)
	$(call ensure-dir,$@)
	@printf "make: $@\n"
	@$(CC) -c -o $@ $< $(CFLAGS) $(DEGUBFLAGS) $(TEST_INCS) $(LIBS)

# Test - Run tests to generate logs
$(OUT_DIR)$(TEST_DIR)%.log: $(OUT_DIR)$(TEST_DIR)%
	@timeout 5 valgrind -q --leak-check=full --track-origins=yes --error-exitcode=117 ./$< > $@ 2>&1; \
	EXIT_CODE=$$?; \
	if [ $$EXIT_CODE -eq 117 ]; then \
		printf "\033[;31mValgrind detected memory errors. Please review $@ for more information\033[;0m\n"; \
	elif [ $$EXIT_CODE -eq 124 ]; then \
		printf "\033[;33mTest timed out after 5s, check for infite loop: $<.\033[;0m\n"; \
	elif [ $$EXIT_CODE -ne 0 ] && [ $$EXIT_CODE -ne 1 ]; then \
		printf "\033[;31mTest failed with exit code $$EXIT_CODE: $<\033[;0m\n"; \
	fi

# Report - Generate report
$(SITE_DIR)index.html: $(GCDAS) $(GCNOS)
	$(call ensure-dir,$(SITE_DIR))
	@gcovr \
		-e $(TEST_DIR) \
		--root . \
		--sort uncovered-percent \
		--html --html-nested --html-theme github.dark-green \
		--html-syntax-highlighting --output $(SITE_DIR)/index.html

# Report - Generate coverage files
$(OBJ_DIR)%.gcda:
	@make $(TEST_LOGS)
$(OBJ_DIR)%.gcno:
	@make $(TEST_LOGS)

.PHONY: default dev prod test report format lint clean

.PRECIOUS: $(OBJS_PROD) $(OBJS_DEV) $(OBJS_TEST) $(OUTS_TEST)
