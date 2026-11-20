DOCKER_IMAGE ?= ghcr.io/yaml/libyaml-dist:latest

build:
	docker build -t $(DOCKER_IMAGE) .

run: build
	docker run --rm -v "$$PWD/../..:/src:z" -it $(DOCKER_IMAGE) bash

dist-docker-local: clean
	docker run --rm -v "$$PWD/../..:/src:z" $(DOCKER_IMAGE) bash /src/pkg/docker/scripts/libyaml-dist.sh

clean:
	rm -rf output/yaml-*.*
