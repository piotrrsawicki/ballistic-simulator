CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lgsl -lgslcblas -lm

TARGET = ballistic_sim
SRCS = main.c missile_sim.c MSIS/nrlmsise-00.c MSIS/nrlmsise-00_data.c egm2008.c date_time.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)