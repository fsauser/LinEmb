DIR_OBJ = .
DIR_BIN = ./bin

OBJ_C = $(wildcard ${DIR_OBJ}/*.c*)
OBJ_O = $(patsubst %.c*,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))

TARGET = argp

CC = g++

DEBUG = -g -O0 -Wall
CFLAGS += $(DEBUG) 

LIB =
$(shell mkdir -p $(DIR_BIN))

${TARGET}:${OBJ_O}
	$(CC) $(CFLAGS) $(OBJ_O) -o $(DIR_BIN)/$@  $(LIB)

${DIR_BIN}/%.o : $(DIR_OBJ)/%.c*
	$(CC) $(CFLAGS) -c  $< -o $@  $(LIB)

clean :
	$(RM) -r $(DIR_BIN) 

