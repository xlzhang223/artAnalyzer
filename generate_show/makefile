DEBUG_FLAGS=-DDEBUG_BUILD -O3 -Wall -std=c++11 -g
RELEASE_FLAGS=-O3 -Wall -std=c++11
COMPILER=g++
SRC=ac_analyzer.cpp   \
    class_method.cpp  \
    options.cpp       \
    manager.cpp       \
    gen_html.cpp

all: release debug

debug: $(SRC)
	$(COMPILER) $(DEBUG_FLAGS) $(SRC) -o debug-analyzer

release: $(SRC)
	$(COMPILER) $(RELEASE_FLAGS) $(SRC) -o release-analyzer

clean:
	rm analyzer
