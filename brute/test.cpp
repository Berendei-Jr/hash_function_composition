#define CL_TARGET_OPENCL_VERSION 300
#include <iostream>
#include <CL/cl.h>

int main() {
    cl_uint platformCount;
    clGetPlatformIDs(0, nullptr, &platformCount);
    std::cout << "Found OpenCL platforms: " << platformCount << std::endl;
    return 0;
}