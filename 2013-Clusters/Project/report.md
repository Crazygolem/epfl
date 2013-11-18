Program parallelization on clusters of PCs — Project: Sieve of Eratosthenes
===========================================================================

Author: Jeremiah Menétrey
Due date: Nov. 20, 2013

Description of the application
------------------------------

The sieve of Eratosthenes is an classic algorithm to find prime numbers up to a given integer. The algorithm works as follows:

Given a list of unmarked integers $[2..L]$ where $L$ is the upper limit of candidate primes
1. Find $n$, the next unmarked number in the list
2. Mark all multiples of $n$, except for $n$ itself
3. Repeat *1-2* until all numbers up to $\sqrt{L}$ have been processed
4. The numbers left unmarked in the list are prime numbers

### Algorithmic complexity ###
The complexity of the algorithm depends on $L$ and the number of primes in the list up to $\sqrt{L}$. It can be proved that the actual complexity is $O(n \log \log n)$, as the prime harmonic series asymptotically approaches $\log \log n$.[^algoComplexity]

### Achievable speedup ###

TODO (Upper bound!)


Parallelization strategies
--------------------------

In this section, it is assumed that every process has a unique ID (called *pid*) and every process knowns its own ID as well as the IDs of all other processes (so they also know the number of processes).
The number of processes is denoted $p$, and processes *pid* is within the range $[1..p]$. There might also be a *master* process, which is not counted in $p$ and has the *pid* $0$.

Both strategies below use the fact that chunks of the list of candidate primes need not to be send over the network. It is sufficient to communicate the upper limit $L$ of the original list, the size of the chunks $S$ or both, to be able to build locally a representation of the chunk with the correct bounds, using the *pid* of the process. Except if stated otherwise, when the communication of a chunk is implied, $L$, the size of the chunks or both are actually sent over the network, depending on the situation.

### First strategy: The easy one ###
The master process computes the primes up to $\sqrt{L}$. Every time it finds a prime, it broadcasts it to all the slaves. When done, it broadcasts a special FIN message indicating that it has finished.

The slaves are each responsible of a different chunk of the candidate primes. When a slave receives a prime from the master, it marks all the multiples of that prime in its chunk. When it receives the FIN message, the slave collects all unmarked numbers (i.e. all primes) in its chunk and sends them back to the master.

#### Discussing the strategy ####
This strategy is very simple and still might achieve good results by letting the master process handle a small portion of the list while allowing to parallelize the processing of the big portion.

This technique scales very well for the portion of the list greater than $\sqrt{L}$. In facts, for a given $L$, the size of the chunk processed by each slave is inversely proportional to the number of slaves. In the extreme case where there are $L - \lfloor \sqrt{L} \rfloor$ slaves (i.e. one slave for each number greater than $\sqrt{L}$), the limiting factor will be given by the rate at which the master will broadcast new primes, which is not parallelized with this strategy.

Another downside of this strategy is the memory limitation: If there are not enough slaves to hold the entire second portion of the list, the algorithm won't work as-is. Caching the primes sent by the master might be a way to overcome the limitation, however the benefits of the on-the-fly sieving (sieving a chunk while the master continuously sends new primes) would not be applicable for all chunks after the first one slaves process.

##### Computational complexity #####
Let $k$ be the number of primes up to $\sqrt{L}$. The prime counting function[^primeCountingFunction] $\pi(n)$ gives the number of primes up to $n$, so we have the relation $k = \pi(L)$. Let's say that $\sqrt{L} = \pi^{-1}(k)$ and stick with $k$ for now.

* Each slave will be handling a chunk of size $S = L/p$, and therefore will be performing $k$ loops over $S$ elements. So the parallel computation time is $k * (L / p)$.
* The master will send $k * p$ messages (broadcasting the primes) and receive $p$ messages (the primes found by the slaves), amounting to a total of $p(k+1)$ messages.
* Computation/Communication ratio: $\frac{kL}{(k+1)p^2} = O(L)$.

The longer the list of candidate primes, the smaller the impact of communications.

##### DPS flow graph #####
TODO


### Second strategy: Incremental sieve ###
This strategy leads to a parallel incremental sieve, where each process is responsible for a chunk of the whole list (which might then theoretically be infinite).

The strategy involves a turn-based approach, where at step $i$ the process responsible of the $i$-th chunk **finishes** that chunk, i.e. it finds all remaining primes in it (by using locally a sieving algorithm).

At step $i$ process with *pid* $i$ performs the finishing sieve on its chunk then broadcasts the newly found primes to all other processes, which perform the sieve on their own chunk using the broadcast primes. Then at step $i+1$, process $i+1$ finishes its own chunk. Again, it broadcasts the newly found primes and all other processes perform the sieve on their own chunk with the primes found by process $i+1$. And so on...

Additionally, each process keeps in memory the whole list of primes found so far, including those broadcast by other processes (which actually amounts to a kind of *shared* list of primes). This allows each process to start processing a new chunk as soon as they found all primes in their chunk. A new chunk is first sieved using the shared list of primes, then the normal processing can resume. When process $p$ has found all primes in his chunk, process $1$ can continue (as if it were the process $p+1$).

If the list of candidate primes is bounded (i.e. there is an upper limit $L$), the algorithm stops after the chunk $l$ that contains $\sqrt{L}$ is finished. When process *pid* $l$ finishes, it sends the new primes to other processes, which in turn perform the sieve using the new primes found by process $l$. But then process $l+1$ does not continue, and instead all processes send to the master their list of remaining primes (unmarked number in their chunk). If needed, one arbitrary process can also send the shared list to the master.

Finally, the master process with *pid* $0$ is only responsible for initiating the sieve, by broadcasting the initial conditions to all other processes, such as the limit $L$, an initial list of primes and the lower bound for the sieve (which is actually the upper bound $L$ of the initial list of primes), the size of the chunks, the number of processes, and other environment variables if needed. If the list of candidate primes is unbounded, process $0$ can also be responsible of collecting occasionally the shared list of primes to save it or display it to the client.

Aside from communications from and to process $0$, which are negligible, this strategy requires only the communication of newly found primes to all processes (the broadcast that updates the shared list of primes), which happens once for every chunk.

If we imagine the setting as a pipeline, it would be a five-stage pipeline with the following stages:

| Stage     | Description                        |
| --------- | ---------------------------------- |
| Update    | Receive new primes and update list |
| Sieve I   | Sieve chunk using updated primes   |
| Sieve II  | Sieve chunk for new primes         |
| Broadcast | Broadcast newly found primes       |
| Sieve III | Sieve new chunk using list         |

Stages *Sieve II*, *Broadcast* and *Sieve III* are only performed by a process when it is its turn to finish a chunk. Non-finishing processes only perform the *Update* and *Sieve I* stages before exiting the pipeline.
Additionally, it must be noted that the *Update* and *Broadcast* stages must happen at the same time, i.e. all non-finishing processes must be in the *Update* stage when the finishing one is in the *Broadcast* stage.
Finally, it can be noted that this design allows for the *Sieve III* stage to take up to the time needed for *Sieve I* and *Sieve II* combined, which might be useful as *Sieve III* is actually the same as *Sieve I* except that it uses all primes found so far instead of only the primes of the last update.
However, since the pipeline is only conceptual, it is possible to relax the *Sieve III* stage by merging it with *Update* and *Sieve I* while buffering incoming broadcasts.

#### Discussing the strategy ####
TODO


Detailed theoretical analysis
-----------------------------

TODO



<!-- Footnodes -->
[^algoComplexity]: [http://en.wikipedia.org/wiki/Sieve_of_Eratosthenes#Algorithm_complexity]()
[^primeCountingFunction]: [http://en.wikipedia.org/wiki/Prime_number#Number_of_prime_numbers_below_a_given_number]()