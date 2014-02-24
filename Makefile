all: lib
	@echo ">>>Successfully built Simple Logger<<<"

cmake:
	@echo "###Begin Generating Makefiles###"
	@echo "Creating build directory..."
	mkdir -p ./build
	@echo "Entering build directory and executing CMake..."
	cd ./build; \
	cmake ..
	@echo "###CMake Complete - Makefiles generated###"

simplog: cmake
	@echo "###Begin Building Simple Logger###"
	@echo "Entering build directory and executing generated Makefile..."
	cd ./build; \
	make
	@echo "###Simple Loger Build Complete###"

lib: simplog
	@echo "Moving library to SimpleLogger root..."
	cp -fv ./build/libsimplog.a .

clean:
	@rm -frv ./build libsimplog.a
