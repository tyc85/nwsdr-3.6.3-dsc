TARGET = fpldpc
CC = g++
CFLAGS = -O3

DLLIB = -ldl

OBJECTS = ArrayLDPC.o

SOURCE = ArrayLDPC_Encoder.cpp ArrayLDPC_Decoder.cpp rngs.cpp rvgs.cpp PerfTest.cpp


  
HEADERS = 

all:
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) -lm $(DLLIB)

clean:
	rm $(TARGET)

compile:
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) -lm

