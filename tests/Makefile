CC ?= cc
GCOV ?= gcov
TAG := $(shell uname -s)-$(shell uname -m)
BUILD := build/$(TAG)
COVERAGE := coverage/$(TAG)
CPPFLAGS := -DTT_TEST_ALL -I"$(abspath ../firmware/Sensors)" -I"$(abspath ../firmware/Trainer)"
CFLAGS := -std=c11 -O0 -g -Wall -Wextra -Werror
SOURCES := test_imu.c test_trainer.c test_recorded.c ../firmware/Sensors/mpu6050.c ../firmware/Trainer/trainer.c ../firmware/Trainer/timing.c
.PHONY: test coverage sanitize clean
test: $(BUILD)/test_trainer
	./$(BUILD)/test_trainer
$(BUILD)/test_trainer: $(SOURCES) $(wildcard ../firmware/Trainer/*.h ../firmware/Sensors/*.h)
	mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -lm -o $@
coverage:
	rm -rf $(COVERAGE)
	mkdir -p $(COVERAGE)
	cd $(COVERAGE) && $(CC) $(CPPFLAGS) $(CFLAGS) --coverage $(addprefix ../../,$(SOURCES)) -lm -o test_trainer
	./$(COVERAGE)/test_trainer
	cd $(COVERAGE) && $(GCOV) -b -c test_trainer-mpu6050.gcno test_trainer-trainer.gcno test_trainer-timing.gcno
sanitize:
	mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer $(SOURCES) -lm -o $(BUILD)/test_sanitize
	./$(BUILD)/test_sanitize
clean:
	rm -rf build coverage
