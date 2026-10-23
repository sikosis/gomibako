APP := gomibako
BUILD_DIR := build
SOURCES := \
	src/GomibakoApp.cpp \
	src/TrashReplicant.cpp \
	src/main.cpp
OBJECTS := $(SOURCES:src/%.cpp=$(BUILD_DIR)/%.o)
RESOURCE := $(BUILD_DIR)/gomibako.rsrc

CXX ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O2 -Isrc
LDFLAGS := -lbe

.PHONY: all clean validate

all: $(APP)

$(APP): $(OBJECTS) $(RESOURCE)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	xres -o $@ $(RESOURCE)
	mimeset -f $@

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RESOURCE): resources/gomibako.rdef | $(BUILD_DIR)
	rc -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

validate:
	./scripts/validate.sh

clean:
	rm -rf $(BUILD_DIR) $(APP)
