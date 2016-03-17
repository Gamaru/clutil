
```
void ParallelFor(const size_t start,
                 const size_t stride,
                 const size_t end,
                 std::function<void (size_t, size_t)> loopBody,
                 IScheduler&& model = StaticScheduler(30))
```

Executes a parallel for loop using all OpenCL devices available. Currently, start should be zero, stride should be 1, and end can be any valid size\_t. The user lamda loopBody should not block, lest the loop serializes. Valid schedulers are StaticScheduler, EGSScheduler, PINAScheduler, or a user defined scheduler that implements the IScheduler interface.

The loop executes all iterations between (start, end). Note the inclusive range. For example, a start of 0 and an end of 5 will execute 6 iterations: 0, 1, 2, 3, 4, 5.

The user lambda receives a start and an end size\_t value for the beginning and end of an iteration chunk. Neither ParallelFor nor the scheduler make any guarantees with regards to the size of a given chunk. The user is responsible for ensuring the lambda executes correctly for arbitrary chunk sizes. Usually, this requires an additional nested serial for loop to further subdivide the chunk. See `examples/MatrixMultiply/Matrix.cc:multiply()` for an example.

Upon return, `ParallelFor()` guarantees that all loop iterations are complete. While `ParallelFor()` modifies the current clUtil device internally, it restores it to its previous state upon return.

For an example of a correct ParallelFor loop, please refer to `examples/ParallelFor`.

# Return #

  * None

# Parameters #

  * start: the starting index for the parallel for loop. Should be zero at present.
  * stride: the stride of the parallel for loop. Should be 1.
  * end: the end
  * loopBody: the user function that defines what the loop should do.
  * model: an Rvalue reference to a scheduler to be used by ParallelFor. The default is a static scheduler with 30 uniform chunks. The Rvalue reference allows users to construct a scheduler inline. The loop must effectively "own" the instantiated scheduler, meaning the scheduler must either be instantiated inline or one must use the `std::move()` function to assign ownership to the loop.