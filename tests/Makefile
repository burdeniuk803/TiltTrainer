CC ?= cc
TAG := $(shell uname -s)-$(shell uname -m)
BUILD := build/$(TAG)
CPPFLAGS := -I../firmware/Sensors -I../firmware/Trainer
CFLAGS := -std=c11 -O0 -g -Wall -Wextra -Werror
.PHONY: test clean
test: $(BUILD)/test_imu
	./$(BUILD)/test_imu
$(BUILD)/test_imu: test_imu.c ../firmware/Sensors/mpu6050.c
	mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -lm -o $@
clean:
	rm -rf build coverage
