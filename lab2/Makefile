FLAGS = -std=c11 -O3 -Wextra -Werror -Wall -Wfloat-equal -pedantic -Iinclude
LIBS = -lm

SRCDIR = src
LIBDIR = lib
BUILDDIR = build

PROGS_SRCS = $(wildcard $(SRCDIR)/*.c)
PROGS = $(patsubst $(SRCDIR)/%.c, %.out, $(PROGS_SRCS))

COMMON_SRCS = $(wildcard $(LIBDIR)/*.c)

PROGS_OBJS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(PROGS_SRCS))
COMMON_OBJS = $(patsubst $(LIBDIR)/%.c, $(BUILDDIR)/%.o, $(COMMON_SRCS))

all: $(BUILDDIR) $(PROGS)

$(BUILDDIR):
	mkdir -p $@

$(PROGS): %.out: $(BUILDDIR)/%.o $(COMMON_OBJS)
	gcc $^ -o $@ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	gcc $(FLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(LIBDIR)/%.c
	gcc $(FLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(PROGS)