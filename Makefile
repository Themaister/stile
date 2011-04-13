TARGET = stile
OBJ = main.o image.o

IMLIB2_CFLAGS = $(shell imlib2-config --cflags)
IMLIB2_LIBS = $(shell imlib2-config --libs)

HEADERS = $(wildcard *.hpp)
CXXFLAGS += -O3 -Wall -g -ansi -pedantic

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(IMLIB2_LIBS)

%.o: %.cpp $(HEADERS)
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(IMLIB2_CFLAGS)


clean:
	rm -f $(TARGET) $(OBJ)


.PHONY: clean
