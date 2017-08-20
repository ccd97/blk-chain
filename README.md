# B-Chain

B-Chain is a simple, minimal version of block-chain to understand the data-structure and working of block-chain.
It is written in C++17. It contains block-chain data-structure as well as network handler.

### Usage

B-Chain requires [GCC](https://gcc.gnu.org/gcc-7/) v7.1+ to compile.
Also, [OpenSSL](https://www.openssl.org/) is required to build.

To compile and run the program

```
$ make
```

### Todos

 - Add locks at critical sections (Currently program crashes due to improper thread synchronization)

### Note

This is just a project to demonstrate blockchain and may not work as a actual block-chain.
