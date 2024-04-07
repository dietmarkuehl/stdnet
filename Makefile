#  Makefile                                                      -*-makefile-*-
#  ----------------------------------------------------------------------------
# 
#  Copyright (c) 2023 Dietmar Kuehl http://www.dietmar-kuehl.de
# 
#  Licensed under the Apache License Version 2.0 with LLVM Exceptions
#  (the "License"); you may not use this file except in compliance with
#  the License. You may obtain a copy of the License at
# 
#    https://llvm.org/LICENSE.txt
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  ----------------------------------------------------------------------------

RM       = rm -f

.PHONY: default build test distclean clean

default: test

stdexec:
	git clone https://github.com/NVIDIA/stdexec

libevent:
	git clone https://github.com/libevent/libevent

test: build
	./build/test_stdnet

build:  stdexec libevent
	@mkdir -p build
	cd build; cmake ..
	cmake --build build

clean:
	$(RM) mkerr olderr *~

distclean: clean
	$(RM) -r build
