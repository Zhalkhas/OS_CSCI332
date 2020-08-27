INCLUDES        = -I. -I/usr/include

LIBS		= libsocklib.a  \
			-ldl -lpthread -lm

COMPILE_FLAGS   = ${INCLUDES} -c 
COMPILE         = gcc  ${COMPILE_FLAGS}
LINK            = gcc -o 

C_SRCS		= \
		producers.c \
		consumers.c \
		passivesock.c \
		connectsock.c \
		prodcon_server.c

SOURCE          = ${C_SRCS}

OBJS            = ${SOURCE:.c=.o}

EXEC		= producers consumers pcserver multiserver pcserver_upd

.SUFFIXES       :       .o .c .h

all		:	library producers consumers pcserver multiserver pcserver_upd

.c.o            :	${SOURCE}
			@echo "    Compiling $< . . .  "
			@${COMPILE} $<

library		:	passivesock.o connectsock.o
			ar rv libsocklib.a passivesock.o connectsock.o

pcserver	:	prodcon_server.o
			${LINK} $@ prodcon_server.o ${LIBS}
multiserver :	multi_echo_server.o
			${LINK} $@ multi_echo_server.o ${LIBS}
pcserver_upd	:	prodcon_server_upd.o
			${LINK} $@ prodcon_server_upd.o ${LIBS}

producers	:	producers.o
			${LINK} $@ producers.o ${LIBS}

consumers	:	consumers.o
			${LINK} $@ consumers.o ${LIBS}

clean           :
			@echo "    Cleaning ..."
			rm -f tags core *.out *.o *.lis *.a ${EXEC} libsocklib.a
