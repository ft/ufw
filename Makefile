EVERYTHING = build-clang-native build-gnu-native
EVERYTHING += build-clang-arm-none-eabi build-gnu-arm-none-eabi
EVERYTHING += build-ti-c2000-c28x

all: $(EVERYTHING)

build-clang-native:
	mkdir -p $@
	cd $@ && cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang ..
	cd $@ && cmake --build .

build-gnu-native:
	mkdir -p $@
	cd $@ && cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
	cd $@ && cmake --build .

build-gnu-arm-none-eabi:
	mkdir -p $@
	cd $@ && cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/gnu-arm-none-eabi.cmake ..
	cd $@ && cmake --build .

build-clang-arm-none-eabi:
	mkdir -p $@
	cd $@ && cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/clang-arm-none-eabi.cmake ..
	cd $@ && cmake --build .

build-ti-c2000-c28x:
	mkdir -p $@
	cd $@ && cmake -DCMAKE_TI_TARGET=c28x \
                       -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/ti-c2000.cmake \
                       -DCMAKE_CXX_COMPILER_ID=TI \
                       ..
	cd $@ && cmake --build .

.PHONY: $(EVERYTHING)
