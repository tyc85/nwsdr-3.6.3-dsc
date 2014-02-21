TARGET = fpldpc

CC = g++

# late we should try -03 and see how it works
# -g flag is removed since it's for debugging 
# -I is to specify the include files path
CFLAGS = -O2 -fPIC  -I. 

LIBLINK = -lm -ldl

OBJECTS = ldpc.o

#OBJECTS=$(SOURCES:.cpp=.o)
LIBS = ArrayLDPC_Encoder.o ArrayLDPC_Decoder.o rngs.o rvgs.o PerfTest.o

SOURCE = ArrayLDPC_Encoder.cpp ArrayLDPC_Decoder.cpp rngs.cpp rvgs.cpp PerfTest.cpp

TEST= test.cpp
  
SHARED_LIB = ldpc.so

all:
#	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) -lm 
	$(CC) $(CFLAGS) -c $(SOURCE) -lm

test:
	g++ -O2 -Wall test.cpp -o test $(LIBLINK)
	
link:$(SHARED_LIB)
	
clean:
	rm $(TARGET) 

	
ldpc.so: $(LIBS)
	gcc -shared -Xlinker -soname=$@ -o $@ -Wl,-whole-archive $^ -Wl,-no-whole-archive -lc

#	g++ -shared PerfTest.o ArrayLDPC_Decoder.o ArrayLDPC_Encoder.o rvgs.o rngs.o -o ldpc.so
