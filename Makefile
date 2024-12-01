export LANG = en_US.UTF-8
export LC_ALL = en_US.UTF-8

.PHONY: test

test: bin/s7 demo/.godot
	@echo 🧪 Running tests
	bin/s7 test/test-main.scm

bin/s7: s7/s7.c
	@echo ⚙️ Building scheme interpreter
	gcc s7/s7.c -o bin/s7 -DWITH_MAIN -DWITH_SYSTEM_EXTRAS -DWITH_C_LOADER=0 -I. -O2 -g -ldl -lm

SRC_FILES := $(shell find src -type f ! -name "*.os")
DEMO_FILES := $(shell find demo -type f -name "*.tscn" -or -name "*.scm")

demo/.godot: $(SRC_FILES) $(DEMO_FILES)
	@echo ⚙️ Building extension
	@scons
	@echo 📦 Importing test scene
	godot --path demo --headless --import

.PHONY: run

run: demo/.godot
	godot -e --path demo main.tscn

.PHONY: android

android:
	scons platform=android target=template_debug

.PHONY: test-watch

test-watch:
	find demo/addons/s7 test | entr make test
