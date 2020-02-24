SOURCES := $(shell find $(SOURCEDIR) -name '*.c')
SOURCES := $(SOURCES) $(shell find $(SOURCEDIR) -name '*.cxx')
HEADERS := $(shell find $(SOURCEDIR) -name '*.h')

all:  $(SOURCES) $(HEADERS)
	clang-format -i $^

clean:
	rm -v smart_parking*.data
