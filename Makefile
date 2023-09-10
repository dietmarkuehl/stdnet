#  Makefile                                                      -*-makefile-*-
#  ----------------------------------------------------------------------------
#   Copyright (C) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
#  ----------------------------------------------------------------------------

RM       = rm -f

.PHONY: default build distclean clean

default: build

stdexec:
        git clone https://github.com/NVIDIA/stdexec

build:  stdexec
	@mkdir -p build
	cd build; cmake ..
	cmake --build build

clean:
	$(RM) mkerr olderr *~

distclean: clean
	$(RM) -r build
