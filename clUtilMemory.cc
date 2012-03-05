#include "clUtilMemory.h"

using namespace clUtil;

void Image::initialize()
{
  cl_int err;

  if(mDimensions == 1) //1D image
  {
    //Map the 1D image to a 2D image
    mWidth = mDevice.getDeviceInfo().Image2DMaxWidth;
    mHeight = m1DWidth % mWidth == 0 ? 
      m1DWidth / mWidth : 
      m1DWidth / mWidth + 1;

    cl_image_format imageFormat = {mChannelOrder, mChannelType};

    mMemHandle = clCreateImage2D(mDevice.getContext(),
                                 CL_MEM_READ_WRITE,
                                 &imageFormat,
                                 mWidth,
                                 mHeight,
                                 0,  
                                 NULL,
                                 &err);
    clUtilCheckError(err);
  }
  else if(mDimensions == 2) //2D image
  {
    cl_image_format imageFormat = {mChannelOrder, mChannelType};

    mMemHandle = clCreateImage2D(mDevice.getContext(),
                                 CL_MEM_READ_WRITE,
                                 &imageFormat,
                                 mWidth,
                                 mHeight,
                                 0,  
                                 NULL,
                                 &err);
    clUtilCheckError(err);
  }
  else if(mDimensions == 3) //3D image
  {
    cl_image_format imageFormat = {mChannelOrder, mChannelType};

    mMemHandle = clCreateImage3D(mDevice.getContext(),
                                 CL_MEM_READ_WRITE,
                                 &imageFormat,
                                 mWidth,
                                 mHeight,
                                 mDepth,
                                 0,
                                 0,
                                 NULL,
                                 &err);
    clUtilCheckError(err);
  }
  else //This should never be reached
  {
    throw clUtilException("Invalid image dimensionality. This should never "
                          "happen; please report this as a bug.");
  }
}

void Image::put(void* pointer, size_t len)
{
  cl_int err;

  if(mDimensions == 1) //1D image
  {
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {mWidth, mHeight, 1};
    size_t pitch;
    size_t imageElementSize;
    
    len = len == 0 ? m1DWidth : len;

    err = clGetImageInfo(mMemHandle,
                         CL_IMAGE_ELEMENT_SIZE,
                         sizeof(imageElementSize),
                         &imageElementSize,
                         NULL);
    clUtilCheckError(err);

    void* mappedImage = clEnqueueMapImage(mDevice.getCommandQueue(),
                                          mMemHandle,
                                          CL_TRUE,
                                          CL_MAP_WRITE,
                                          origin,
                                          region,
                                          &pitch,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          &err);
    clUtilCheckError(err);

    memcpy(mappedImage, pointer, imageElementSize * len);

    err = clEnqueueUnmapMemObject(mDevice.getCommandQueue(),
                                  mMemHandle,
                                  mappedImage,
                                  0,
                                  NULL,
                                  NULL);
  }
  else if(mDimensions == 2 || mDimensions == 3)
  {
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {mWidth, mHeight, mDepth};

    err = clEnqueueWriteImage(mDevice.getCommandQueue(),
                              mMemHandle,
                              CL_TRUE,
                              origin,
                              region,
                              0,
                              0,
                              pointer,
                              0,
                              NULL,
                              NULL);
    clUtilCheckError(err);
  }
  else
  {
    throw clUtilException("Invalid image dimensionality. This should never "
                          "happen; please report this as a bug.");
  }
}

void Image::get(void* pointer, size_t len)
{
  cl_int err;

  if(mDimensions == 1) //1D image
  {
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {mWidth, mHeight, 1};
    size_t pitch;
    size_t imageElementSize;
    
    len = len == 0 ? m1DWidth : len;

    err = clGetImageInfo(mMemHandle,
                         CL_IMAGE_ELEMENT_SIZE,
                         sizeof(imageElementSize),
                         &imageElementSize,
                         NULL);
    clUtilCheckError(err);

    void* mappedImage = clEnqueueMapImage(mDevice.getCommandQueue(),
                                          mMemHandle,
                                          CL_TRUE,
                                          CL_MAP_READ,
                                          origin,
                                          region,
                                          &pitch,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          &err);
    clUtilCheckError(err);

    memcpy(pointer, mappedImage, imageElementSize * len);

    err = clEnqueueUnmapMemObject(mDevice.getCommandQueue(),
                                  mMemHandle,
                                  mappedImage,
                                  0,
                                  NULL,
                                  NULL);
  }
  else if(mDimensions == 2 || mDimensions == 3)
  {
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {mWidth, mHeight, mDepth};

    err = clEnqueueReadImage(mDevice.getCommandQueue(),
                             mMemHandle,
                             CL_TRUE,
                             origin,
                             region,
                             0,
                             0,
                             pointer,
                             0,
                             NULL,
                             NULL);
    clUtilCheckError(err);
  }
  else
  {
    throw clUtilException("Invalid image dimensionality. This should never "
                          "happen; please report this as a bug.");
  }
}

void Buffer::put(void* pointer, size_t len)
{
  cl_int err;

  if(len == 0)
  {
    len = mLength;
  }

  err = clEnqueueWriteBuffer(mDevice.getCommandQueue(),
                             mMemHandle,
                             CL_TRUE,
                             0,
                             len,
                             pointer,
                             0,
                             NULL,
                             NULL);
  clUtilCheckError(err);
}

void Buffer::get(void* pointer, size_t len)
{
  cl_int err;

  if(len == 0)
  {
    len = mLength;
  }

  err = clEnqueueReadBuffer(mDevice.getCommandQueue(),
                            mMemHandle,
                            CL_TRUE,
                            0,
                            len,
                            pointer,
                            0,
                            NULL,
                            NULL);
  clUtilCheckError(err);
}