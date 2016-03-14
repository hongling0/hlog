.PHONY: all clean
# http://blog.csdn.net/brooknew/article/details/8452358
INC=-I. -I./include/
CFLAGS+=$(INC) -g -O0 -Wall -lpthread


CONF_TEST_SOURCE=src/hconf.c src/hpool.c test/test_conf.c
CONF_TEST_TARGET=./bin/test_conf

LOCK_TEST_SOURCE=test/test_lock.c
LOCK_TEST_TARGET=./bin/test_lock

LIB_SOURCE=$(wildcard src/*.c)
LIB_TARGET=./lib/libhlog.a
OBJS=$(patsubst %.c,%.o,$(LIB_SOURCE))

TEST=CONF_TEST LOCK_TEST


all:$(LIB_TARGET) $(foreach prog,$(TEST),$($(prog)_TARGET))

$(LIB_TARGET):$(OBJS)
	$(AR) $(ARFLAGS) $@ $^

define calc_depends
$($(1)_TARGET):$(patsubst %.c,%.o,$($(1)_SOURCE))
	$$(CC) $$(CFLAGS) $$(CPPFLAGS) $$^ -o $$@

OBJS+=$(patsubst %.c,%.o,$($(1)_SOURCE))
endef

$(foreach prog,$(TEST),$(eval $(call calc_depends,$(prog))))

DEPS=$(patsubst %.o,%.d,$(OBJS))


%.o:%.c %.d

ifneq ($(MAKECMDGOALS),clean)
%.d:%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< -MT$(@:.d=.o) -MF $@
endif

-include $(DEPS)

echo:
	@echo $(LIB_SOURCE)
	@echo $(OBJS)
	@echo $(DEPS)
	@echo $(TEST_TARGET)
	@echo $(LIB_TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) $(TEST_TARGET)
	$(RM) $(LIB_TARGET)
	@echo $(LIB_SOURCE)
