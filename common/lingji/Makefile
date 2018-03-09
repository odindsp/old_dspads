CFLAGS = -Wall -g -O2

base_dir = /home/supertool/dengxiaolu/opt/
INCLUDE_PATH  = -I ./ 
LIB_PATH = -L${base_dir}/lib
SRC_CPP = $(wildcard *.cpp)
SRC_CC = $(wildcard *.cc)

LIB = -lpthread -lssl -lcrypto -lrt -ldl -lz -pthread

OBJ = $(patsubst %.cpp,%.o,$(SRC_CPP)) $(patsubst %.cc,%.o,$(SRC_CC))

#OBJX = ${base_dir}/lib/libconfig++.a \
	${base_dir}/lib/libprotobuf.a

decode:$(OBJ) $(OBJX)
	@echo linking $^ ...
	@echo g++ $^ -o $@ $(CFLAGS) $(INCLUDE_PATH) $(LIB)
	@g++ $^ -o $@ $(CFLAGS) $(INCLUDE_PATH) $(LIB) 
%.o:%.cpp
	@echo compling $^ ...
	@g++ -c $^ -o $@ $(CFLAGS) $(INCLUDE_PATH)
clean:
	@rm $(OBJ) decode
