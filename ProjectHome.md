This project seeks to vastly improve user productivity when using OpenCL by assuming away or simplifying many more complex and esoteric use models supported by OpenCL. This allows for significantly terser code and significantly reduces the number of objects the user must keep track of. Furthermore, clUtil turns OpenCL's error checking nightmare into a more manageable problem by using exception handling.

As an example, this
main.cc
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
    float array[2000];
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
  
    //Free everything else
    clReleaseContext(context);
    clReleaseCommandQueue(commandQueue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);

    return 0;
}
```

becomes this:

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

using namespace clUtil;

int main(int argc, char** argv)
{
    const char* kernelFiles[] = {"kernel.cl"};
    const unsigned int kLength = 2000;
    float array[kLength];
    float val = 20.0f;

    Device::InitializeDevices(kernelFiles, 1);
    Buffer buffer(sizeof(array));

    clUtilEnqueueKernel("fill", clUtilGrid(length, 64), buffer, length, val);
    
    buffer.get(array);

    return 0;
}
```


---


Requirements:
  * OpenCL 1.1 or greater
  * An OpenCL supported device
  * A compiler supporting at least the following C++0x features: rvalues, variadic templates (g++ 4.4.1 or later, Visual C++ Nov 2012 CTP)