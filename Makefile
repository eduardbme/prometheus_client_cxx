.PHONY: tests examples

tests:
	docker build -t prometheus_client_cxx_tests -f ./tests/Dockerfile . --progress=plain && \
	docker run --rm prometheus_client_cxx_tests

examples:
	docker build -t prometheus_client_cxx_examples -f ./examples/Dockerfile . --progress=plain && \
	docker run -it --rm -p 19100:19100 prometheus_client_cxx_examples

format:
	./clang-format-all .
