# Compiler
CC = gcc

# Source files
SRCS = lexan.c  

# Object files
OBJS = $(SRCS:.c=.o) 

# Executable name
TARGET = lexan

# Rule to link object files and create the executable

$(TARGET): $(OBJS)
	gcc -o splitter splitter.c Hash.c
	gcc -o builder builder.c Hash.c
	$(CC) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@

# Execute the program  GreatExpectations.txt
run:
	./$(TARGET) -i GreatExpectations.txt -l 100 -m 150 -t 10 -e ExclusionList.txt -o output.txt

# For the memory leaks
valgrind:
	valgrind --leak-check=full ./$(TARGET)


# Remove the object files
clean:
	rm -f $(OBJS) $(TARGET)