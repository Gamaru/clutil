

# Introduction #

If any of you have used OpenCL, you'll know that running a program in OpenCL is anything but easy. Consider the following example that fills an array with a specified value.
main.cc:
```
#include <Opencl/cl.h>

char const* kernelSource = "__kernel void fill(__global float* array, unsigned int arrayLength, float val)"
"{"
"    if(get_global_id(0) < arrayLength)"
"    {"
"        array[get_global_id(0)] = val;}"
"    }"
"}";

int main(int argc, char** argv)
{
    float val = 20.0f;
    float array[2000]
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue commandQueue;
    cl_mem buffer;
    cl_program program;
    cl_kernel kernel;
    unsigned int length = 2000;

    //Initialization
    err = clGetPlatform_IDs(1, &platform, NULL);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ANY, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    commandQueue = clCreateCommandQueue(context, device, 0, &err);
    program = clCreateProgramWithSource(context, 1, &kernelSource, 0, &err);
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "fill", &err);

    //Allocate memory    
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(array), NULL, &err);

    //Actually call the kernel
    err = clSetKernelArg(kernel, 0, sizeof(buffer), &buffer);
    err = clSetKernelArg(kernel, 1, sizeof(length), &length);
    err = clSetKernelArg(kernel, 2, sizeof(val), &val);

    size_t global;
    size_t local = 64;
    
    global = length % local == 0 ? length : (length / local + 1) * local;

    err = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &global, &local, 0, NULL, NULL);

    //Copy data back
    err = clEnqueueReadBuffer(commandQueue, buffer, CL_TRUE, 0, sizeof(array), array, 0, NULL, NULL);

    //Free the data on the GPU
    clReleaseMemObject(buffer);
}
```

This program doesn't even do error checking, which would at least double the program's length! clUtil dramatically improves frontend expressiveness without significantly hurting flexibility. This means you write less code, spend less time debugging, and more time getting results.

The equivalent program in clUtil is as follows:

kernel.cl:
```
__kernel void fill(__global float* array, unsigned int arrayLength, float val)
{
    if(get_global_id(0) < arrayLength)
    {
        array[get_global_id(0)] = val;
    }
}
```
main.cc:
```
#include <clUtil.h>

int main(int argc, char** argv)
{
    const char* kernelFiles[] = {"kernel.cl"};
    const unsigned int kLength = 2000;
    float array[kLength];
    float val = 20.0f;

    Device::InitializeDevices(kernelFiles, 1);
    Buffer buffer(sizeof(array));

    clUtilEnqueueKernel("fill", clUtilGrid(length, 64), buffer, length, val);
    clUtilDeviceGet(array, sizeof(array), buffer);

    return 0;
}
```

# Design #

Simplifying the OpenCL API requires several assumptions and design decisions that trade expressivity for flexibility. This being said, clUtil still provides enough options for most applications. These assumptions include:
Simplified device/platform model
Simpler memory model
Stronger programming model
clUtil uses many features not available in C such as parameter defaulting and STL objects. Additionally, it uses variadic templates for launching kernels. Because of these language features, clUtil requires a C++ compiler with at least variadic template support of the C++0x standard. As such, one of the following compilers is required:
  * g++ 4.3 or later (use -std=c++0x flag on any file that includes clUtil.h). We have tested g++ 4.4.1, 4.5.2, and 4.6 with x86\_64-linux-gnu target.
We currently support Linux running OpenCL 1.1 or later. At present, we have tested clUtil using ATI's StreamSDK, Nvidia's CUDA, and Intel's OpenCL SDK (though, I can't remember the nonsense you have to do with libnuma to avoid a segfault).

## Device Model ##

clUtil makes no distinction between device platforms. Devices are devices and will implicitly run on the correct platform. On initialization, clUtil queries every platform available and combines all their device listings into a unified device list with the following assumptions (which will likely be loosened in the future):
  * Each device has a single context.
  * Each device has a two command queues, switched at programmer discretion. Both queues execute operations in order, but only because not every SDK supports OOO execution.

clUtil has a simple C++ RAII friendly memory model. Users allocate buffers by creating buffer objects. Image objects embody 1, 2, and 3-D images. When calling their copy constructor, clUtil calls clRetainMemObject. When they destruct, clUtil calls clReleaseMemObject.

clUtil's memory model provides fewer options than vanilla OpenCL. All buffers are allocated on the currently selected device's only context. All buffers are read/write. Host pointer capabilities exist through passing additional parameters to a buffer's constructor.

clUtil adds convenience functions for representing matrices as 2D images. Furthermore, clUtil adds support for 1D images by emulating them using 2D images. Additional functions exist in clUtil to create, copy to, and copy from 1-D images. They are visible to kernels as image1d\_t types and are read and written to using [language extensions](http://code.google.com/p/clutil/wiki/Documentation#1D_Image_Routines).

## Programming model ##

clUtil makes dramatically simplifies OpenCL's programming model. Developers no longer have to worry about kernels, programs, binaries, etc. clUtil takes a list of source code files at initialization and compiles them all for each device on the machine. Calling a kernel is as simple as calling [clUtilEnqueueKernel()] after initialization with the kernel name, work dimension, global work size, local work size, and the arguments to the kernel.

Since compiling source code can be take a while, developers can optionally pass a list of binary names to the initialization routine, causing clUtil to load kernels from these files if they exist. If they don't exist, clUtil will compile from source and save these binary files. This means that many applications can cache their programs after the first time the application is run.

### Synchronicity ###

All clUtil data transfer and kernel enqueue functions execute asynchronously. The runtime manages cl\_event objects on behalf of the user to ensure correct execution order regardless of which queue a task executes from. These events also can optionally provide profiling information.

## Thread safety ##

clUtil is not currently thread safe. We have no plans to make it so, as our model consists of a sequential master thread issuing tasks to devices. If absolutely need thread safety, you can put mutex locks around regions that assume a specific current device. For example:

```
mutex_lock(...);

Device::SetCurrentDevice(...);

buffer.put(...);
clUtilEnqueueKernel(...);
buffer.get(...);

mutex_unlock(...);
```

Since all transfer and kernel functions execute asynchronously, the amount of time spent in the critical section should be minimal.

# User Guide #

A typical OpenCL, CUDA, or clUtil application has a motif similar to the following:
  1. Initialize
  1. Allocate memory
  1. Copy data to device
  1. Run kernels
  1. Copy data from device
  1. Repeat 3-5
  1. Shutdown

## Initialization/Device Managment ##

Users populate the list of available devices by calling [Device::FetchDevices()](FetchDevices.md). This populates the device list and fetches device info for each device. Users can then get the list of devices and examine their information (and that's about it). This function allows for pre-initialization computation to discover which devices the program should use for what, how to use them, and some other advanced topics.

To get a STL vector of the available [devices](Device.md), call [Device::GetDevices()](GetDevices.md). This vector is immutable.

To set the current device, call [Device::SetCurrentDevice(size\_t num)](SetCurrentDevice.md)

To query the current device, call [Device::GetCurrentDevice()](GetCurrentDevice.md)

To get the cardinal number of the current device, call [Device::GetCurrentDeviceNum()](GetCurrentDeviceNum.md).

Calling the [getDeviceInfo()](getDeviceInfo.md) member function returns a structure containing all the queryable features obtainable through `clGetDeviceInfo()`.

The features available by this pre-initialization step are very limited. To perform full initialization, the user calls [Device::InitializeDevices()](InitializeDevices.md). Calling this function will implicitly call [Device::FetchDevices()](FetchDevices.md) is not already called. This function accepts a list of OpenCL source code files, and optionally an object file cache name and additional compiler options.

[Device::InitializeDevices()](InitializeDevices.md) creates contexts on each device, creates two command queues, compiles the source objects into a program, and fetches kernel objects from said program, storing them in a data structure. After calling this function, users are ready to allocate buffers on devices and enqueue kernels and data transfers.

## Memory Management ##

[Buffer](Buffer.md) and [Image](Image.md) objects back buffers and images residing on the GPU. By virtue of instantiating one of these objects, the clUtil runtime calls `clCreateBuffer()` or `clCreate{2|3}DImage()`. Both of these objects inherit from the abstract [Memory](Memory.md) class, which contains the underlying memory handle and access event. The Memory class also manages reference counting via its copy constructors (which call `clRetainMemObject()`) and destructors (which call `clReleaseMemObject()`).

[Memory](Memory.md) objects are exception safe; if an exception is thrown, all buffer objects presently in scope will do the right thing according to OpenCL's reference counting mechanism (which may or may not release the underlying memory if there exist valid copies of the object). This highlights the importance of using `shared_ptr<>` and `unique_ptr<>` (both reside in STL as of C++11) data types, as these pointers add exception safety for dynamically allocated objects.

To transfer data between device and host, call the [get()](get.md) member function.

To transfer data between host and device, call the [put()](put.md) member function.

When calling either [get()](get.md) or [put()](put.md), the clUtil runtime passes the existing `cl_event` object as a dependency to `clEnqueue{Read|Write}{Buffer|Image}()` and updates the `cl_event` associated with the [Memory](Memory.md) object behind the scenes with the returned event. This guarantees correctness when executing tasks in multiple queues (or in the future, out of order queues).

## Error handling ##

clUtil manages errors through exceptions. If something anomalous occurs, in a clUtil library function, the runtime throws a [clUtilException](clUtilException.md). Inheriting from `std::exception`, users can use the `what()` member function to get a human readable string description of the error that occurred. In the future, we plan to be better people, and create an exception class for every exception type, allowing for more interesting try/catch blocks. However, for the time being, using a catchall for [clUtilException](clUtilException.md) or even `std::exception` provides sufficient notification for debugging.

For those of you not aware, in gdb you can call "catch throw" and gdb will break whenever any exception is thrown. This is useful for getting stack traces and finding the exact point in time an error occurs, as well as why it occurred.

## Kernels ##

To launch a kernel, call [clUtilEnqueueKernel()](clUtilEnqueueKernel.md) with the name of the kernel, a [clUtilGrid()](clUtilGrid.md) call, and arguments to the kernel. This function looks up the kernel by name in a hash table for the current device that is created when clUtil is initialized. It automatically recognizes Buffer objects, Image objects, and pointers to Memory objects and passes the underlying `cl_mem` handle instead. Other arguments are passed verbatim along with their size to `clSetKernelArg()`.

[clUtilEnqueueKernel()](clUtilEnqueueKernel.md) is variadic and accepts any number of arguments greater than or equal to 2.

[clUtilGrid()](clUtilGrid.md) is a variadic function that creates an object defining the thread execution. Arguments must come in pairs of two, the first argument being a global work size and the second being a local work size. For example, a 3D grid with 128x64x32 global work items with 8x4x2 local work items would call:

```
clUtilGrid(128, 8, 64, 4, 32, 2)
```

[clUtilGrid()](clUtilGrid.md) is useful only to [clUtilEnqueueKernel()](clUtilEnqueueKernel.md) and rvalue references allow it to be used inline. There is no compelling reason to keep track of the complex variadic templated objects this function returns (though, this is possible at the expense of ugly code).

Because OpenCL requires that global work sizes be a multiple of local work sizes, clUtil will round each dimension UP to the nearest multiple of the corresponding work size. Any reasonably designed kernel should check if extraneous threads exist and deny them execution.

The [clUtilEnqueueKernel()](clUtilEnqueueKernel.md) function implicitly reads the `cl_event`s associated with Memory objects, creates a wait list with these objects, and passes this to `clEnqueueNDRangeKernel()`. The runtime then updates each Memory object to associate with the returned `cl_event`. This dependency tracking allows code to correctly run with multiple command queues without the need for barriers and markers. Furthermore, we can turn on out of order execution in queues when more SDKs support this and still guarantee sequential correctness.

## Platform/Device Specific Compilation ##
clUtil passes the preprocessor define `CLUTIL_DEVICE_ID=k`, where k is the cardinal device number of the device it's currently compiling your program for. This allows your OpenCL kernel to do fancy per-device compilation. Consider the following:

constants.cl:
```
#pragma once

#if CLUTIL_DEVICE_ID == 1
#define CALL_FOO
#elif CLUTIL_DEVICE_ID == 2
#define CALL_BAR
#endif
```

prog.cl:
```
#include <constants.cl>

__kernel myFunction()
{
//...

#if defined(CALL_FOO)
  foo();
#elif defined(CALL_BAR)
  bar();
#else
#error Unknown calling convention
#endif
//...

}
```

In this example, constants.cl is a header file created at runtime after calling [Device::FetchDevices()](FetchDevices.md) and using parameters in the device's info to decide how to execute the program. This allows you to optimize code on a device-by-device basis. You can use similar tricks to stream data through different kinds of memory (global, local, or images) and decide which to do at runtime.

Another useful feature is the CLUTIL\_DEVICE\_EMBARGO environment variable. This variable takes a colon delimited list of devices that will be unseen to clUtil. This allows you to quickly omit devices from the device pool without recompiling any code. For example, if you want to test the performance on a GPU, you can create an embargo list of every other device.

## Parallel For ##
Oftentimes, users wish to use multiple OpenCL devices to collectively solve a problem. clUtil provides the [ParallelFor()](ParallelFor.md) function for this purpose. This function takes the start, stride (presently ignored), and stop loop indices, a lambda containing the loop body, and an optional loop scheduler (the static scheduler is default). An example of such a loop for vector addition:

```
ParallelFor(0, 1, kBigArraySize - 1, [&](size_t startIdx, size_t endIdx)
{ 
  unsigned int indexCount = endIdx - startIdx + 1;
  size_t deviceNum = Device::GetCurrentDeviceNum();

  aDevice[deviceNum]->put(&a[startIdx], indexCount * sizeof(float));
  bDevice[deviceNum]->put(&b[startIdx], indexCount * sizeof(float));

  clUtilEnqueueKernel("vectorAdd", 
                      clUtilGrid(indexCount, 64),
                      *aDevice[deviceNum],
                      *bDevice[deviceNum],
                      *cDevice[deviceNum],
                      indexCount);

  cDevice[deviceNum]->get(&c[startIdx], indexCount * sizeof(float));
});
```

The lambda executes sequentially on the main thread, and as such all operations in the user lambda must be non-blocking. All [put](put.md)/[get](get.md) operations as well as [clUtilEnqueueKernel()](clUtilEnqueueKernel.md) are non-blocking. Do not put calls to [Device::Barrier()](Barrier.md) inside the loop body (save for debugging purposes) or the loop will serialize. When the loop returns, the clUtil runtime guarantees that all loop iterations have completed. Futhermore, the [ParallelFor](ParallelFor.md) loop will restore the current device number to its prior state before execution.

One important note is that the [ParallelFor](ParallelFor.md) operation is heterogeneous: it will use every device available in your system for computation. However, it inherently respects the CLUTIL\_DEVICE\_EMBARGO environment variable.

An optional fifth parameter is an rvalue reference to a scheudler object. clUtil presently supports three options [StaticScheduler()](StaticScheduler.md), [EGSScheduler()](EGSScheduler.md), and [PINAScheduler()](PINAScheduler.md). The first scheduler accepts an optional number of chunks (default is 30), and breaks the loop iterations into roughly equal sized tasks. The [EGSScheduler](EGSScheduler.md) just sucks. Don't use it (if you must know, it's an enhanced version of the guided self-scheduling algorithm that reserves larger chunks for faster devices). The [PINAScheduler](PINAScheduler.md) uses autotuning to find the appropriate chunk sizes for each device type.

To facilitate autotuning, the [PINAScheduler](PINAScheduler.md) requires the programmer name the loop (done via its first parameter).

When autotuning, the PINA scheduler will print a message to stdout notifying the user that the a loop is being tuned. When the user no longer sees any of these messages, the loops are fully tuned. During the tuning process, the user should give representative problems to the application so it can discover actual amortization. Application developers can perform autotuning on behalf of the user by writing representative loops with accompanying data sets as separate applications whose loop names match those in the actual application. Scripts can repeat execution until the message no longer appears. Then the user is free to run their application with any data set.

In addiction to autotuning, the PINA scheduler uses runtime chunk timing information to decide which chunks a given device should execute. This allows each device to predict which chunks it will execute most quickly over other devices. In the future, the PINA scheduler can also use its autotuning data to actually exclude very slow devices from the loop execution.

## Profiling ##