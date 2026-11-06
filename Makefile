APP := gomibako
BUILD_DIR := build
INSTALL_DIR ?= /boot/home/config/non-packaged/apps
SOURCES := \
	src/AboutWindow.cpp \
	src/AutoEmptyWarningWindow.cpp \
	src/GomibakoApp.cpp \
	src/Settings.cpp \
	src/SettingsWindow.cpp \
	src/TrashIcon.cpp \
	src/TrashReplicant.cpp \
	src/main.cpp
OBJECTS := $(SOURCES:src/%.cpp=$(BUILD_DIR)/%.o)
RESOURCE := $(BUILD_DIR)/gomibako.rsrc
RESOURCE_INPUTS := \
	resources/gomibako.rdef \
	resources/icon.png \
	resources/icon-32.icon \
	resources/icon-16.icon

CXX ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O2 -Isrc
LDFLAGS := -lbe -ltranslation

.PHONY: all clean icons install validate

all: $(APP)

$(APP): $(OBJECTS) $(RESOURCE)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	xres -o $@ $(RESOURCE)
	mimeset -f $@

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RESOURCE): $(RESOURCE_INPUTS) | $(BUILD_DIR)
	rc -o $@ resources/gomibako.rdef

$(BUILD_DIR):
	mkdir -p $@

icons:
	python3 scripts/generate_legacy_icons.py

install: $(APP)
	mkdir -p "$(INSTALL_DIR)"
	cp $(APP) "$(INSTALL_DIR)/$(APP)"
	mimeset -f "$(INSTALL_DIR)/$(APP)"

validate:
	./scripts/validate.sh

clean:
	rm -rf $(BUILD_DIR) $(APP)
