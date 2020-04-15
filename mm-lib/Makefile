CC = gcc
AR = ar
CFLAGS = -g -O2
OBJ = fake.o handle_module.o
LIB = libmodule_adaptor.a
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

.PHONY: all clean

all: $(LIB)

%.o: %.c
%.o: %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $(OUTPUT_OPTION) $<
$(DEPDIR): ; @mkdir -p $@
DEPFILES := $(OBJ:%.o=$(DEPDIR)/%.d)
$(DEPFILES):

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

clean: 
	rm -f *.o *.a
include $(wildcard $(DEPFILES))
