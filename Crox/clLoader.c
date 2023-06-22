#include "framework_cl.h"



clGetPlatformIDs_fn	clGetPlatformIDs;
clGetPlatformInfo_fn clGetPlatformInfo;
clGetDeviceIDs_fn	clGetDeviceIDs;
clGetDeviceInfo_fn	clGetDeviceInfo;
clCreateContext_fn	clCreateContext;
clCreateContextFromType_fn clCreateContextFromType;
clRetainContext_fn	clRetainContext;
clReleaseContext_fn	clReleaseContext;
clGetContextInfo_fn	clGetContextInfo;
clRetainCommandQueue_fn clRetainCommandQueue;
clReleaseCommandQueue_fn clReleaseCommandQueue;
clGetCommandQueueInfo_fn clGetCommandQueueInfo;
clCreateBuffer_fn	clCreateBuffer;
clRetainMemObject_fn	clRetainMemObject;
clReleaseMemObject_fn clReleaseMemObject;
clGetSupportedImageFormats_fn clGetSupportedImageFormats;
clGetMemObjectInfo_fn clGetMemObjectInfo;
clGetImageInfo_fn	clGetImageInfo;
clRetainSampler_fn	clRetainSampler;
clReleaseSampler_fn	clReleaseSampler;
clGetSamplerInfo_fn	clGetSamplerInfo;
clCreateProgramWithSource_fn clCreateProgramWithSource;
clCreateProgramWithBinary_fn clCreateProgramWithBinary;
clRetainProgram_fn	clRetainProgram;
clReleaseProgram_fn	clReleaseProgram;
clBuildProgram_fn	clBuildProgram;
clGetProgramInfo_fn	clGetProgramInfo;
clGetProgramBuildInfo_fn clGetProgramBuildInfo;
clCreateKernel_fn	clCreateKernel;
clCreateKernelsInProgram_fn clCreateKernelsInProgram;
clRetainKernel_fn	clRetainKernel;
clReleaseKernel_fn	clReleaseKernel;
clSetKernelArg_fn	clSetKernelArg;
clGetKernelInfo_fn	clGetKernelInfo;
clGetKernelWorkGroupInfo_fn clGetKernelWorkGroupInfo;
clWaitForEvents_fn	clWaitForEvents;
clGetEventInfo_fn	clGetEventInfo;
clRetainEvent_fn		clRetainEvent;
clReleaseEvent_fn	clReleaseEvent;
clGetEventProfilingInfo_fn clGetEventProfilingInfo;
clFlush_fn			clFlush;
clFinish_fn			clFinish;
clEnqueueReadBuffer_fn clEnqueueReadBuffer;
clEnqueueWriteBuffer_fn clEnqueueWriteBuffer;
clEnqueueCopyBuffer_fn clEnqueueCopyBuffer;
clEnqueueReadImage_fn clEnqueueReadImage;
clEnqueueWriteImage_fn clEnqueueWriteImage;
clEnqueueCopyImage_fn clEnqueueCopyImage;
clEnqueueCopyImageToBuffer_fn clEnqueueCopyImageToBuffer;
clEnqueueCopyBufferToImage_fn clEnqueueCopyBufferToImage;
clEnqueueMapBuffer_fn clEnqueueMapBuffer;
clEnqueueMapImage_fn clEnqueueMapImage;
clEnqueueUnmapMemObject_fn clEnqueueUnmapMemObject;
clEnqueueNDRangeKernel_fn clEnqueueNDRangeKernel;
clEnqueueNativeKernel_fn clEnqueueNativeKernel;
clSetCommandQueueProperty_fn clSetCommandQueueProperty;
clCreateImage2D_fn clCreateImage2D;
clCreateImage3D_fn clCreateImage3D;
clEnqueueMarker_fn clEnqueueMarker;
clEnqueueWaitForEvents_fn clEnqueueWaitForEvents;
clEnqueueBarrier_fn clEnqueueBarrier;
clUnloadCompiler_fn clUnloadCompiler;
clGetExtensionFunctionAddress_fn clGetExtensionFunctionAddress;
clCreateCommandQueue_fn clCreateCommandQueue;
clCreateSampler_fn clCreateSampler;
clEnqueueTask_fn clEnqueueTask;
clCreateSubBuffer_fn clCreateSubBuffer;
clSetMemObjectDestructorCallback_fn clSetMemObjectDestructorCallback;
clCreateUserEvent_fn clCreateUserEvent;
clSetUserEventStatus_fn clSetUserEventStatus;
clSetEventCallback_fn clSetEventCallback;
clEnqueueReadBufferRect_fn clEnqueueReadBufferRect;
clEnqueueWriteBufferRect_fn clEnqueueWriteBufferRect;
clEnqueueCopyBufferRect_fn clEnqueueCopyBufferRect;
clCreateSubDevices_fn clCreateSubDevices;
clRetainDevice_fn clRetainDevice;
clReleaseDevice_fn clReleaseDevice;
clCreateImage_fn clCreateImage;
clCreateProgramWithBuiltInKernels_fn clCreateProgramWithBuiltInKernels;
clCompileProgram_fn clCompileProgram;
clLinkProgram_fn clLinkProgram;
clUnloadPlatformCompiler_fn clUnloadPlatformCompiler;
clGetKernelArgInfo_fn clGetKernelArgInfo;
clEnqueueFillBuffer_fn clEnqueueFillBuffer;
clEnqueueFillImage_fn clEnqueueFillImage;
clEnqueueMigrateMemObjects_fn clEnqueueMigrateMemObjects;
clEnqueueMarkerWithWaitList_fn clEnqueueMarkerWithWaitList;
clEnqueueBarrierWithWaitList_fn clEnqueueBarrierWithWaitList;
clGetExtensionFunctionAddressForPlatform_fn clGetExtensionFunctionAddressForPlatform;
clCreateCommandQueueWithProperties_fn clCreateCommandQueueWithProperties;
clCreatePipe_fn clCreatePipe;
clGetPipeInfo_fn clGetPipeInfo;
clSVMAlloc_fn clSVMAlloc;
clSVMFree_fn clSVMFree;
clCreateSamplerWithProperties_fn clCreateSamplerWithProperties;
clSetKernelArgSVMPointer_fn clSetKernelArgSVMPointer;
clSetKernelExecInfo_fn clSetKernelExecInfo;
clEnqueueSVMFree_fn clEnqueueSVMFree;
clEnqueueSVMMemcpy_fn clEnqueueSVMMemcpy;
clEnqueueSVMMemFill_fn clEnqueueSVMMemFill;
clEnqueueSVMMap_fn clEnqueueSVMMap;
clEnqueueSVMUnmap_fn clEnqueueSVMUnmap;
clSetDefaultDeviceCommandQueue_fn clSetDefaultDeviceCommandQueue;
clGetDeviceAndHostTimer_fn clGetDeviceAndHostTimer;
clGetHostTimer_fn clGetHostTimer;
clCreateProgramWithIL_fn clCreateProgramWithIL;
clCloneKernel_fn clCloneKernel;
clGetKernelSubGroupInfo_fn clGetKernelSubGroupInfo;
clEnqueueSVMMigrateMem_fn clEnqueueSVMMigrateMem;
clSetProgramSpecializationConstant_fn clSetProgramSpecializationConstant;
clSetProgramReleaseCallback_fn clSetProgramReleaseCallback;
clSetContextDestructorCallback_fn clSetContextDestructorCallback;
clCreateBufferWithProperties_fn clCreateBufferWithProperties;
clCreateImageWithProperties_fn clCreateImageWithProperties;
clGetGLContextInfoKHR_fn clGetGLContextInfoKHR;
clCreateFromGLBuffer_fn clCreateFromGLBuffer;
clCreateFromGLTexture_fn clCreateFromGLTexture;
clCreateFromGLRenderbuffer_fn clCreateFromGLRenderbuffer;
clGetGLObjectInfo_fn clGetGLObjectInfo;
clGetGLTextureInfo_fn	clGetGLTextureInfo;
clEnqueueAcquireGLObjects_fn clEnqueueAcquireGLObjects;
clEnqueueReleaseGLObjects_fn clEnqueueReleaseGLObjects;
clCreateFromGLTexture2D_fn clCreateFromGLTexture2D;
clCreateFromGLTexture3D_fn clCreateFromGLTexture3D;

#ifdef cl_khr_gl_event

clCreateEventFromGLsyncKHR_fn clCreateEventFromGLsyncKHR;

#endif // cl_khr_event

#if cl_intel_sharing_format_query_gl

clGetSupportedGLTextureFormatsINTEL_fn clGetSupportedGLTextureFormatsINTEL;

#endif // cl_khr_gl_depth_images



int clLoadCLUserPtr(CLADuserptrloadfunc load, void* userptr)
{

}