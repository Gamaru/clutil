#pragma once
#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <mach/mach_time.h>
#include <mach/mach.h>
#include <CoreServices/CoreServices.h>
#else
#include <CL/cl.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <new>
#include <map>
#include <string>
#include <functional>

#define kCLUtilMaxDevices 64
#define kCLUtilMaxKernelNameLength 128

#define clUtilCheckErrorVoid(err)\
{\
  if(err != CL_SUCCESS)\
  {\
    printf("%s:%d::%s\n", __FILE__, __LINE__, clUtilGetErrorCode(err));\
    raise(SIGTRAP);\
    return;\
  }\
}

#define clUtilCheckError(err)\
{\
  if(err != CL_SUCCESS)\
  {\
    printf("%s:%d::%s\n", __FILE__, __LINE__, clUtilGetErrorCode(err));\
    raise(SIGTRAP);\
    return err;\
  }\
}

typedef struct _DeviceSize_t
{
  size_t deviceWidth;
  union
  {
    size_t host;
    long long device;
    unsigned int devicePart[2];
  };
}DeviceSize_t;

class clMemPointer
{
  public:
    cl_mem mBase;
    int mOffset;
    clMemPointer(const clMemPointer& copy)
    {
      mBase = copy.mBase;
      mOffset = copy.mOffset;
    }
    clMemPointer(cl_mem base, int offset)
    {
      mBase = base;
      mOffset = offset;
    }

    //Operators
    clMemPointer operator[](int offset)
    {
      return clMemPointer(mBase, mOffset + offset);
    }
    clMemPointer operator=(cl_mem base)
    {
      mBase = base;
      mOffset = 0;
      return clMemPointer(mBase, mOffset);
    }
    clMemPointer operator+=(int offset)
    {
      mOffset += offset;
      return clMemPointer(mBase, mOffset);
    }
    clMemPointer operator-=(int offset)
    {
      mOffset -= offset;
      return clMemPointer(mBase, mOffset);
    }

  protected:
    clMemPointer();
};

namespace clUtil
{
  extern cl_device_id gDevices[kCLUtilMaxDevices];
  extern cl_context gContexts[kCLUtilMaxDevices];
  extern cl_program gPrograms[kCLUtilMaxDevices];
  extern cl_command_queue gCommandQueues[kCLUtilMaxDevices];
  extern cl_uint gNumDevices;
  extern unsigned int gCurrentDevice;
  extern cl_kernel* gKernels[kCLUtilMaxDevices];
  extern std::map<std::string, cl_kernel> gKernelNameLookup[kCLUtilMaxDevices];
};

//typedef void (*clUtilCallback)(cl_event, cl_int);
typedef std::function<void (cl_event, cl_int)> clUtilCallback;

//Run user callback...
void clUtilRunLambda(cl_event event,
                     cl_int status,
                     void* user_data);

//Init and management crap
cl_int clUtilInitialize(const char** filenames, 
                        size_t numFiles, 
                        const char* cachename = NULL,
                        const char* options = NULL);
cl_int clUtilSetDeviceNum(cl_int device);
cl_int clUtilGetDeviceNum();
cl_uint clUtilGetNumDevices();
cl_program clUtilGetProgram();
cl_command_queue clUtilGetCommandQueue();
cl_kernel clUtilGetKernel(std::string& kernelName, cl_int* err);

//Get info crap
cl_int clUtilGetPointerSize();
size_t clUtilGetMaxBlockSize();
char* clUtilGetDeviceName();
char* clUtilGetDeviceVendor();
char* clUtilGetDeviceDriver();
char* clUtilGetPlatformVersion();
cl_uint clUtilGetMaxWriteImages();
void clUtilGetSupportedImageFormats();

//Turns error code into string
const char* clUtilGetErrorCode(cl_int err);

//---Memory crap---
cl_int clUtilAlloc(size_t bytes, cl_mem* gpuBuffer);

//Synchronous
cl_int clUtilDevicePut(void* buffer, size_t len, cl_mem gpuBuffer);

//Asynchronous
cl_int clUtilDevicePut(void* buffer, 
                       size_t len,
                       cl_mem gpuBuffer,
                       clUtilCallback&& callback);

cl_int clUtilDeviceGet(void* buffer, size_t len, cl_mem gpuBuffer);
cl_int clUtilFree(cl_mem buffer);

//Image crap
cl_int clUtilPutImage1D(cl_mem image,
                        const size_t offset,
                        const size_t region,
                        void* ptr);

cl_int clUtilGetImage1D(cl_mem image,
                        const size_t offset,
                        const size_t region,
                        void* ptr);

cl_int clUtilCreateImage1D(size_t numPixels,
                           cl_channel_order order,
                           cl_channel_type type,
                           cl_mem* image);

cl_int clUtilCopyToImageFloat(cl_mem buffer, 
                              int offset,
                              int m,
                              int n,
                              int ld,
                              cl_mem* image);
cl_int clUtilCopyToImageTransposeFloat(cl_mem buffer, 
                                       int offset,
                                       int m,
                                       int n,
                                       int ld,
                                       cl_mem* image);
cl_int clUtilCopyToImageDouble(cl_mem buffer, 
                               int offset,
                               int m,
                               int n,
                               int ld,
                               cl_mem* image);
cl_int clUtilCopyToImageTransposeDouble(cl_mem buffer, 
                                        int offset,
                                        int m,
                                        int n,
                                        int ld,
                                        cl_mem* image);
cl_int clUtilCopyToImageFloat4(cl_mem buffer, 
                               int offset,
                               int m,
                               int n,
                               int ld,
                               cl_mem* image);
cl_int clUtilCopyToImageTransposeFloat4(cl_mem buffer, 
                                        int offset,
                                        int m,
                                        int n,
                                        int ld,
                                        cl_mem* image);
cl_int clUtilCopyToImageDouble2(cl_mem buffer, 
                                int offset,
                                int m,
                                int n,
                                int ld,
                                cl_mem* image);
cl_int clUtilCopyToImageTransposeDouble2(cl_mem buffer, 
                                         int offset,
                                         int m,
                                         int n,
                                         int ld,
                                         cl_mem* image);
cl_int clUtilDebugPrintImageFloat(cl_mem image);
cl_int clUtilDebugPrintImageDouble(cl_mem image);

//Timing
double clUtilGetTime();
