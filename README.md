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

## References

- [TAPS Architecture](https://datatracker.ietf.org/doc/draft-ietf-taps-arch/)
- [TAPS Interface](https://datatracker.ietf.org/doc/draft-ietf-taps-interface/)
- [TAPS Implementation](https://datatracker.ietf.org/doc/draft-ietf-taps-impl/)
- [Secure Networking in C++](http://wg21.link/P1861)
- [Sender/Receiver for Networking](http://wg21.link/p2762)
- [Networking based on IETF_TAPS](https://github.com/rodgert/papers/blob/master/source/p3185.bs)
- [python-asyncio-taps](https://github.com/fg-inet/python-asyncio-taps)
