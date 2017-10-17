LIBS = -lcurl

TARGET = backdoor knocker

all: $(TARGET)

backdoor: backdoor.cpp
	g++ -o backdoor backdoor.cpp $(LIBS)

knocker: knocker.cpp
	g++ -o knocker knocker.cpp

clean:
	rm $(TARGET)
