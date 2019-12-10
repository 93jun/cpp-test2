CXX		= g++
OPT		= -std=c++11
SRCS		= $(wildcard *.cpp) $(wildcard *.c)
OBJS		= $(SRCS:.cpp=.o)
TARGET		= out


all : $(TARGET)
	$(CXX) $(OPT) -o $(TARGET) $(OBJS) 

$(TARGET) :
	$(CXX) $(OPT) -c $(SRCS)

clean :
	rm -f $(TARGET)
	rm -f *.o
