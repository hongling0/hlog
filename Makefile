.PHONY: all clean cleanlog
INC=-I. -I./include/
CFLAGS+=$(INC) -g -O3 -Wall -L./lib -lhlog -lpthread
ARFLAGS=rcus


LIB_SOURCE=$(wildcard src/*.c) $(wildcard plugin/*.c)
LIB_TARGET=./lib/libhlog.a
OBJS=$(patsubst %.c,%.o,$(LIB_SOURCE))


TEST_CONF_SOURCE=test/test_conf.c
TEST_CONF_TARGET=./bin/test_conf

TEST_LOCK_SOURCE=test/test_lock.c
TEST_LOCK_TARGET=./bin/test_lock

TEST_LOG_SOURCE=test/test_log.c
TEST_LOG_TARGET=./bin/test_log

TEST=TEST_CONF TEST_LOCK TEST_LOG


all:$(LIB_TARGET) $(foreach prog,$(TEST),$($(prog)_TARGET))

$(LIB_TARGET):$(OBJS)
	$(AR) $(ARFLAGS) $@ $^

define calc_depends
$($(1)_TARGET):$(patsubst %.c,%.o,$($(1)_SOURCE)) $(LIB_TARGET)
	$$(CC) $$^ $$(CFLAGS) $$(CPPFLAGS) -o $$@

OBJS+=$(patsubst %.c,%.o,$($(1)_SOURCE))
TEST_TARGET+=$($(1)_TARGET)
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

cleanlog:
	$(RM) tmp/*log*	

clean:
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) $(TEST_TARGET)
	$(RM) $(LIB_TARGET)
