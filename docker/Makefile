
build:
	docker build -t libyaml-dev .

run: build
	docker run -it --rm libyaml-dev bash

libyaml-dist: build
	rm -rf output/libyaml.git
	git clone ../../.git output/libyaml.git
	docker run -it --rm -u $$(id -u) -v$$PWD/output:/output libyaml-dev ./libyaml-dist.sh

test-pyyaml: build
	rm -rf output/libyaml.git
	git clone ../../.git output/libyaml.git
	docker run -it --rm -v$$PWD/output:/output libyaml-dev ./libyaml-test-pyyaml.sh
