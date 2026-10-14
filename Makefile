GCOV ?= gcov
IMAGE ?= stm32c0-wokwi-build:local

.PHONY: image build clean test coverage sanitize test-linux coverage-linux

image:
	docker build -t $(IMAGE) docker

build: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace \
		--user "$(shell id -u):$(shell id -g)" $(IMAGE) \
		make -C firmware all

clean:
	$(MAKE) -C firmware clean


test:
	python3 tools/check_project.py
	$(MAKE) -C tests test
coverage:
	$(MAKE) -C tests coverage GCOV="$(GCOV)"
sanitize:
	$(MAKE) -C tests sanitize
test-linux: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace --user "$(shell id -u):$(shell id -g)" $(IMAGE) make test
coverage-linux: image
	docker run --rm -v "$(CURDIR):/workspace" -w /workspace --user "$(shell id -u):$(shell id -g)" $(IMAGE) make coverage GCOV=gcov
