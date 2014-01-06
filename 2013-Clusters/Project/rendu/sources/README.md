Running the programs
====================

All programs have an advanced set of options. Here are only described the basic commands to launch the programs with acceptable default parameters.

For more advanced usages, the launching scripts can be good hints, otherwise the source code of the applications themselves should not be too hard to understand, or at least the parameters should be easy to find.

All commands assume that the sources are correctly compiled with DPS, and that the environment paths are properly configured.


Serial sieve
------------

Find the primes up to ```n```:

```Serial <n>```


Parallel sieve
--------------

Find the primes up to ```n``` with ```s``` slaves (a hosts file with enough working hosts must be provided):

```mpiexec -mapall -machinefile <hosts file> -n <s+1> Parallel -slaves <s> -max <n>```


Network tests
-------------

In addition to the sieve programs, two version of a program targeted at testing network speed and latency are provided. One version outputs the RTT, the second version is modified to output approximatively half the RTT by preparing and sending messages from the distant node.

Both provide the same interface. To have the program send ```m``` concurrent messages with a payload of ```p``` integers:

```mpiexec -mapall -machinefile <hosts file> -n <m+1> Test -slaves <m> -payload <p>```