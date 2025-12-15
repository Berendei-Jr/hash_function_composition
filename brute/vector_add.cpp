#define CL_TARGET_OPENCL_VERSION 300
#include <iostream>
#include <vector>
#include <CL/cl.h>

const char* kernelSource = R"(
__kernel void vector_add(__global const float* a,
                         __global const float* b,
                         __global float* result) {
    int gid = get_global_id(0);
    result[gid] = a[gid] + b[gid];
}
)";

int main() {
    // Данные
    const size_t N = 1024;
    std::vector<float> a(N, 1.0f);
    std::vector<float> b(N, 2.0f);
    std::vector<float> result(N);

    // Получаем платформу
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);

    // Получаем устройство
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, nullptr);

    // Создаем контекст и очередь команд
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, 0, nullptr);

    // Создаем буферы
    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, N * sizeof(float), nullptr, nullptr);
    cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, N * sizeof(float), nullptr, nullptr);
    cl_mem bufResult = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N * sizeof(float), nullptr, nullptr);

    // Копируем данные в буферы
    clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, N * sizeof(float), a.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, N * sizeof(float), b.data(), 0, nullptr, nullptr);

    // Создаем программу и ядро
    cl_program program = clCreateProgramWithSource(context, 1, &kernelSource, nullptr, nullptr);
    clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    cl_kernel kernel = clCreateKernel(program, "vector_add", nullptr);

    // Устанавливаем аргументы ядра
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufResult);

    // Запускаем ядро
    size_t globalSize = N;
    clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &globalSize, nullptr, 0, nullptr, nullptr);

    // Читаем результат
    clEnqueueReadBuffer(queue, bufResult, CL_TRUE, 0, N * sizeof(float), result.data(), 0, nullptr, nullptr);

    // Проверяем результат
    bool success = true;
    for (size_t i = 0; i < N; ++i) {
        if (result[i] != 3.0f) {
            success = false;
            break;
        }
    }
    std::cout << (success ? "Тест пройден!" : "Ошибка!") << std::endl;

    // Освобождаем ресурсы
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufResult);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}