# Compiler
CXX = g++

# Folder settings
SRCDIR = ./src
INCDIR = ./src

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic

# Source files
SRCS := $(filter-out $(SRCDIR)/main.cpp, $(wildcard $(SRCDIR)/*.cpp))
OBJS := $(SRCS:.cpp=.o)

# Executable name
TARGET = ims #TODO change

# Build directory
BUILDDIR = .

all: $(TARGET)

$(TARGET): $(OBJS) $(SRCDIR)/main.cpp
	$(CXX) $(CFLAGS) -I$(INCDIR) $(LIBS) $(OBJS) $(INCDIR)/main.cpp -o $(BUILDDIR)/$@

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.hpp
	$(CXX) $(CFLAGS) -I$(INCDIR) -c $< -o $@


.PHONY: clean
.PHONY: run

clean:
	rm -rf $(OBJS) $(TARGET)

run:
	./$(TARGET)

