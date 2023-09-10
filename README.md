# Standard C++ Networking

Demo implementation of C++ networking based on the [Networking
TS](http://wg21.link/n4771.pdf) and the [sender/receiver for
networking proposal](http://wg21.link/p2762).

## Building

The implementation requires [stdexec](https://github.com/NVIDIA/stdexec)
for the implementation of the sender library. It may need to be cloned
into the root directory:

    git clone https://github.com/NVIDIA/stdexec

When using the `Makefile` the repository should be cloned automatically.
