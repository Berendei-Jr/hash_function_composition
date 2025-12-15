#define CL_TARGET_OPENCL_VERSION 300
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include <CL/cl.h>
#include <sys/stat.h>
#include <chrono>
#include "main.cl.h"


cl_command_queue queue;
cl_mem buf_input;
cl_mem buf_output;
cl_mem buf_size;
const size_t num_data = 1024*1024*8;

// Вывод хеша в hex
void printHash(const uint8_t* hash, uint8_t len) {
    for (int i = 0; i < len; i++) {
        printf("%02X", hash[i]);
    }
    printf("\n");
}

bool compareHash(const uint8_t* h1, const uint8_t* h2, uint8_t len){
    for (uint8_t i = 0; i < len; i++) {
        if (h1[i] != h2[i]) {
            return false;
        }
    }
    return true;
}

// Вспомогательные функции
char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = (char*)malloc(length + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    fread(content, 1, length, file);
    content[length] = '\0';
    fclose(file);
    return content;
}

int file_exists(const char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Компиляция отдельного модуля
cl_program compile_module(cl_context context, cl_device_id device, 
                         const char* source_code, const char* options) {
    cl_int err;
    
    // Создаем программу из исходного кода
    cl_program program = clCreateProgramWithSource(context, 1, &source_code, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Error creating program: %d\n", err);
        return NULL;
    }
    
    // Компилируем программу (но не линкуем!)
    err = clCompileProgram(program, 1, &device, options, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error compiling program: %d\n", err);
        
        // Получаем лог компиляции
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Compilation log:\n%s\n", log);
        free(log);
        
        clReleaseProgram(program);
        return NULL;
    }
    
    return program;
}

// Линковка скомпилированных модулей
cl_program link_modules(cl_context context, cl_device_id device,
                       cl_program* modules, int num_modules,
                       const char* options) {
    cl_int err;
    
    printf("Linking %d modules...\n", num_modules);
    
    // Линкуем все модули вместе
    cl_program linked_program = clLinkProgram(context, 1, &device, options,
                                             num_modules, modules, NULL, NULL, &err);
    
    if (err != CL_SUCCESS) {
        printf("Error linking programs: %d\n", err);
        
        // Получаем лог линковки
        size_t log_size;
        clGetProgramBuildInfo(linked_program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(linked_program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Link log:\n%s\n", log);
        free(log);
        
        return NULL;
    }
    
    return linked_program;
}

// Компиляция с кешированием скомпилированных модулей
cl_program compile_with_caching(cl_context context, cl_device_id device,
                               const char* source_filename, 
                               const char* cache_filename,
                               const char* options) {
    
    // Проверяем существование кеша
    if (file_exists(cache_filename)) {
        printf("Loading cached module: %s\n", cache_filename);
        
        // Загружаем бинарник из файла
        char* binary_data = read_file(cache_filename);
        if (binary_data) {
            size_t binary_size = strlen(binary_data);
            cl_int binary_status, err;
            
            cl_program program = clCreateProgramWithBinary(context, 1, &device, 
                                                          &binary_size, 
                                                          (const unsigned char**)&binary_data,
                                                          &binary_status, &err);
            free(binary_data);
            
            if (err == CL_SUCCESS && binary_status == CL_SUCCESS) {
                // Компилируем загруженный бинарник
                err = clCompileProgram(program, 1, &device, options, 0, NULL, NULL, NULL, NULL);
                if (err == CL_SUCCESS) {
                    return program;
                }
            }
        }
    }
    
    // Компилируем из исходников
    printf("Compiling from source: %s\n", source_filename);
    char* source_code = read_file(source_filename);
    if (!source_code) {
        return NULL;
    }
    
    cl_program program = compile_module(context, device, source_code, options);
    free(source_code);
    
    if (program) {
        // Сохраняем бинарное представление (для демонстрации - сохраняем исходник)
        // В реальном приложении нужно использовать clGetProgramInfo с CL_PROGRAM_BINARIES
        FILE* cache_file = fopen(cache_filename, "wb");
        if (cache_file) {
            source_code = read_file(source_filename);
            if (source_code) {
                fwrite(source_code, 1, strlen(source_code), cache_file);
                free(source_code);
            }
            fclose(cache_file);
        }
    }
    
    return program;
}

void Hash( std::vector<cl_uint>* data_ptr, int size, cl_kernel* kernel_ptr)
{
    cl_int err;
    std::vector<cl_uint> data = *data_ptr;
    std::vector<cl_uint> size_data(1, 0);
    std::vector<cl_uint> output_data(num_data * BUFFER_SIZE, 0);
    cl_kernel kernel_sha512 = *kernel_ptr;
    for (size_t i = 0; i < num_data; i++) {
        printf("%d: ", i);
        printHash((uint8_t*)(&data[i*BUFFER_SIZE]), BUFFER_SIZE*4);
    }

    err = clEnqueueWriteBuffer(queue, buf_input, CL_TRUE, 0,
                            data.size() * sizeof(cl_uint), data.data(),
                            0, nullptr, nullptr);
    err = clEnqueueWriteBuffer(queue, buf_size, CL_TRUE, 0,
                            size_data.size() * sizeof(cl_uint), size_data.data(),
                            0, nullptr, nullptr);
    
    err = clSetKernelArg(kernel_sha512, 0, sizeof(cl_mem), &buf_input);
    err |= clSetKernelArg(kernel_sha512, 1, sizeof(cl_mem), &buf_output);
    err |= clSetKernelArg(kernel_sha512, 2, sizeof(cl_mem), &buf_size);
    
    size_t global_size = num_data;
    err = clEnqueueNDRangeKernel(queue, kernel_sha512, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);
    
    // 6. Получение результатов
    err = clEnqueueReadBuffer(queue, buf_input, CL_TRUE, 0,
                            output_data.size() * sizeof(cl_uint), output_data.data(),
                            0, nullptr, nullptr);
    for (size_t i = 0; i < num_data; i++) {
        printf("%d: ", i);
        printHash((uint8_t*)(&output_data[i*BUFFER_SIZE]), BUFFER_SIZE*4);
    }
}

int main() {
    cl_int err;
    
    const int data_size = 4;
    
    // 1. Подготовка OpenCL
    cl_platform_id platform;
    cl_device_id device;
    cl_context context; 
    
    err = clGetPlatformIDs(1, &platform, nullptr);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
    
    auto start_time = std::chrono::high_resolution_clock::now();


    // Опции компиляции
    const char* compile_options = "-cl-fast-relaxed-math -Werror";
    
    // Компилируем библиотечные модули (с кешированием)
    cl_program md5_lib = compile_with_caching(context, device,
                                              "inc_hash_md5.cl", 
                                              "inc_hash_md5.cl.bin",
                                              compile_options);
    
    cl_program platform_lib = compile_with_caching(context, device,
                                              "inc_platform.cl", 
                                              "inc_platform.cl.bin",
                                              compile_options);

    cl_program common_lib = compile_with_caching(context, device,
                                              "inc_common.cl", 
                                              "inc_common.cl.bin",
                                              compile_options);
    cl_program sha512_lib = compile_with_caching(context, device,
                                              "inc_hash_sha512.cl", 
                                              "inc_hash_sha512.cl.bin",
                                              compile_options);
    // cl_program sha256_lib = compile_with_caching(context, device,
    //                                           "inc_hash_sha256.cl", 
    //                                           "inc_hash_sha256.cl.bin",
    //                                           compile_options);
    
    if (!md5_lib || !platform_lib || !common_lib || !sha512_lib) {
        printf("Failed to compile libraries\n");
        return -1;
    }
    
    printf("Libraries compiled successfully\n");
    
    char* main_source = read_file("main.cl");
    if (!main_source) {
        printf("Failed to read main kernel_sha512\n");
        return -1;
    }
    
    cl_program main_program = compile_module(context, device, main_source, compile_options);
    free(main_source);
    
    if (!main_program) {
        printf("Failed to compile main program\n");
        return -1;
    }
    
    printf("Main program compiled successfully\n");
    
    // Линкуем все модули вместе
    cl_program modules[] = {md5_lib, platform_lib, common_lib, main_program, sha512_lib};
    cl_program final_program = link_modules(context, device, modules, 5, compile_options);
    
    if (!final_program) {
        printf("Failed to link programs\n");
        return -1;
    }
    
    printf("All modules linked successfully\n");
    
    cl_kernel kernel_sha512 = clCreateKernel(final_program, "simple_sha512", &err);
    cl_kernel kernel_md5 = clCreateKernel(final_program, "simple_md5", &err);
    cl_kernel kernel_xor = clCreateKernel(final_program, "simple_xor", &err);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    printf("kernel_sha512 created successfully: %d sec\n", duration.count()/1000);
    
    // 3. Подготовка данных
    std::vector<cl_uint> input_data(num_data*BUFFER_SIZE, 0);
    std::vector<cl_uint> size_data(1, 0);
    std::vector<cl_uint> output_data(num_data * BUFFER_SIZE, 0);

    // 4. Создание буферов
    buf_input = clCreateBuffer(context, CL_MEM_READ_WRITE, 
            input_data.size() * sizeof(cl_uint), nullptr, &err);
    cl_mem buf_size = clCreateBuffer(context, CL_MEM_READ_WRITE,
            size_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v4 = clCreateBuffer(context, CL_MEM_READ_WRITE, input_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v4_size = clCreateBuffer(context, CL_MEM_READ_WRITE, size_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v0 = clCreateBuffer(context, CL_MEM_READ_WRITE, input_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v0_size = clCreateBuffer(context, CL_MEM_READ_WRITE, size_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v1 = clCreateBuffer(context, CL_MEM_READ_WRITE, input_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v1_size = clCreateBuffer(context, CL_MEM_READ_WRITE, size_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v2 = clCreateBuffer(context, CL_MEM_READ_WRITE, input_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v2_size = clCreateBuffer(context, CL_MEM_READ_WRITE, size_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v3 = clCreateBuffer(context, CL_MEM_READ_WRITE, input_data.size() * sizeof(cl_uint), nullptr, &err);
cl_mem buf_v3_size = clCreateBuffer(context, CL_MEM_READ_WRITE, size_data.size() * sizeof(cl_uint), nullptr, &err);

bool is_founded = false;
    uint32_t iteration = 0;
    uint32_t last_itaration = 100000;
    // uint8_t target_hash[] = {137, 247, 170, 71, 105, 201, 65, 123, 229, 39, 67, 30, 13, 170, 77, 167}; //54321 md5 

    // uint8_t target_hash[] = {41, 131, 89, 19, 153, 215, 172, 212, 184, 1, 49, 21, 126, 124, 71, 132, 
    //     46, 27, 119, 7, 30, 193, 168, 174, 249, 62, 132, 195, 72, 200, 246, 8, 153, 40, 192, 63, 255, 
    //     38, 11, 221, 53, 166, 49, 196, 180, 160, 251, 238, 72, 138, 141, 221, 49, 105, 229, 76, 201, 
    //     105, 160, 62, 21, 77, 180, 236}; // 54321 sha512

    // uint8_t target_hash[] = {33, 159, 112, 147, 129, 43, 178, 7, 129, 41, 76, 235, 202, 184, 62, 92, 
    //     215, 29, 158, 248, 198, 125, 27, 45, 156, 62, 7, 47, 236, 111, 31, 237, 122, 69, 189, 33, 221, 
    //     64, 51, 177, 156, 140, 47, 37, 49, 117, 88, 90, 56, 130, 227, 33, 141, 33, 251, 180, 82, 239, 
    //     143, 243, 176, 120, 112, 187}; // 6969 sha512 

    // uint8_t target_hash[] = {14, 29, 8, 119, 160, 48, 234, 53, 51, 177, 44, 201, 109, 90, 180, 29}; //54321 sha512 -> md5

    // uint8_t target_hash[] = {160, 116, 243, 84, 240, 30, 237, 175, 93, 38, 114, 11, 115, 214, 10, 35,
    //     46, 27, 119, 7, 30, 193, 168, 174, 249, 62, 132, 195, 72, 200, 246, 8, 153, 40, 192, 63, 255,
    //     38, 11, 221, 53, 166, 49, 196, 180, 160, 251, 238, 72, 138, 141, 221, 49, 105, 229, 76, 201,
    //     105, 160, 62, 21, 77, 180, 236}; //xor(md5(54321), sha512(54321))
    

    // uint8_t target_hash[] = {15, 239, 25, 213, 197, 118, 87, 141, 32, 77, 241, 148, 147, 16, 108, 119, 104, 149, 236, 185, 138, 115, 75, 169, 188, 35, 240, 0, 115, 176, 219, 8, 118, 222, 70, 134, 44, 74, 110, 162, 184, 164, 192, 44, 17, 53, 13, 86, 101, 155, 239, 156, 172, 214, 48, 126, 254, 204, 93, 185, 213, 208, 64, 71};
    // uint8_t target_hash[] = {8, 101, 137, 242, 1, 130, 176, 41, 34, 222, 209, 188, 175, 50, 170, 239, 91, 163, 191, 244, 197, 237, 138, 145, 92, 134, 33, 2, 79, 1, 61, 209, 169, 19, 57, 235, 139, 72, 211, 164, 55, 98, 13, 73, 254, 146, 139, 35, 138, 8, 136, 18, 238, 133, 127, 246, 87, 148, 30, 239, 245, 1, 85, 193};
    // uint8_t target_hash[] = {0, 167, 6, 66, 199, 49, 24, 64, 237, 32, 52, 94, 46, 32, 72, 34, 151, 123, 174, 192, 8, 169, 67, 38, 32, 107, 24, 247, 133, 26, 27, 64, 185, 35, 31, 13, 107, 243, 84, 35, 159, 149, 31, 231, 74, 211, 67, 209, 177, 90, 221, 102, 165, 182, 236, 221, 156, 240, 129, 174, 176, 69, 92, 48};
    // uint8_t target_hash[] = {33, 12, 217, 146, 231, 136, 179, 66, 97, 146, 209, 105, 94, 91, 112, 168, 164, 141, 34, 254, 46, 206, 199, 120, 78, 134, 38, 228, 169, 25, 234, 119, 36, 208, 143, 60, 117, 37, 67, 116, 96, 245, 193, 27, 231, 237, 125, 54, 230, 25, 206, 25, 180, 32, 201, 5, 170, 106, 244, 213, 215, 7, 49, 162};
    // uint8_t target_hash[] = {33, 69, 235, 190, 80, 18, 209, 243, 50, 181, 2, 54, 21, 163, 86, 21, 20, 56, 230, 239, 226, 78, 169, 136, 216, 124, 8, 169, 150, 5, 11, 186, 86, 23, 39, 35, 44, 39, 93, 46, 112, 161, 179, 10, 110, 185, 41, 103, 77, 19, 64, 104, 97, 148, 103, 56, 221, 43, 87, 213, 241, 33, 21, 0};
    // uint8_t target_hash[] = {53, 183, 5, 91, 18, 226, 253, 207, 221, 211, 238, 9, 19, 1, 5, 211, 1, 108, 210, 152, 21, 140, 187, 197, 236, 209, 161, 65, 51, 130, 105, 6, 248, 187, 191, 228, 95, 156, 238, 168, 207, 131, 240, 217, 8, 114, 221, 131, 211, 207, 157, 74, 255, 166, 158, 161, 117, 235, 22, 232, 232, 162, 112, 109};
    uint8_t target_hash[] = {31, 170, 185, 181, 210, 39, 72, 40, 255, 117, 96, 90, 121, 201, 145, 63, 13, 202, 13, 142, 164, 251, 209, 132, 128, 48, 104, 21, 231, 143, 163, 137, 178, 196, 163, 143, 222, 67, 248, 235, 66, 125, 101, 252, 253, 105, 79, 57, 215, 130, 146, 138, 28, 228, 200, 242, 40, 164, 38, 160, 24, 254, 152, 165};
    
    size_t global_size = num_data;

    start_time = std::chrono::high_resolution_clock::now();
    while (!is_founded){
        if (iteration == last_itaration) break;
        // printf("it: %d\n", iteration);
        // Заполняем входные данные
        for (size_t i = 0; i < num_data; i++) {
            long long num = i + iteration*num_data;
            for (int j = 0; j < BUFFER_SIZE; j++)
            {
                input_data[i*BUFFER_SIZE+j] = num&0xffffffff;
                num = num >> 32;
            }
        }
        
        
err = clEnqueueWriteBuffer(queue, buf_v4, CL_TRUE, 0,
                                input_data.size() * sizeof(cl_uint), input_data.data(),
                                0, nullptr, nullptr);
size_data[0]=data_size;
err = clEnqueueWriteBuffer(queue, buf_v4_size, CL_TRUE, 0,
                                size_data.size() * sizeof(cl_uint), size_data.data(),
                                0, nullptr, nullptr);clSetKernelArg(kernel_sha512, 0, sizeof(cl_mem), &buf_v4);
clSetKernelArg(kernel_sha512, 1, sizeof(cl_mem), &buf_v4_size);
clSetKernelArg(kernel_sha512, 2, sizeof(cl_mem), &buf_v0);
clSetKernelArg(kernel_sha512, 3, sizeof(cl_mem), &buf_v0_size);
clEnqueueNDRangeKernel(queue, kernel_sha512, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);clSetKernelArg(kernel_md5, 0, sizeof(cl_mem), &buf_v4);
clSetKernelArg(kernel_md5, 1, sizeof(cl_mem), &buf_v4_size);
clSetKernelArg(kernel_md5, 2, sizeof(cl_mem), &buf_v1);
clSetKernelArg(kernel_md5, 3, sizeof(cl_mem), &buf_v1_size);
clEnqueueNDRangeKernel(queue, kernel_md5, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);clSetKernelArg(kernel_xor, 0, sizeof(cl_mem), &buf_v0);
clSetKernelArg(kernel_xor, 1, sizeof(cl_mem), &buf_v0_size);
clSetKernelArg(kernel_xor, 2, sizeof(cl_mem), &buf_v1);
clSetKernelArg(kernel_xor, 3, sizeof(cl_mem), &buf_v1_size);
clSetKernelArg(kernel_xor, 4, sizeof(cl_mem), &buf_v2);
clSetKernelArg(kernel_xor, 5, sizeof(cl_mem), &buf_v2_size);
clEnqueueNDRangeKernel(queue, kernel_xor, 1, nullptr, &global_size, nullptr, 0, nullptr, nullptr);clEnqueueReadBuffer(queue, buf_v2, CL_TRUE, 0,
                                output_data.size() * sizeof(cl_uint), output_data.data(),
                                0, nullptr, nullptr);
clEnqueueReadBuffer(queue, buf_v2_size, CL_TRUE, 0,
                                size_data.size() * sizeof(cl_uint), size_data.data(),
                                0, nullptr, nullptr);
 // Hash(&input_data, data_size, &kernel_sha512);
        
        // 7. Вывод результатов
        for (size_t i = 0; i < num_data; i++) {
            // printf("%03d: ", i + iteration*num_data);
            // printHash((uint8_t*)(&output_data[i*BUFFER_SIZE]), size_data[0]);
            // if (i + iteration*num_data == 1000)
            // {
            //     printHash((uint8_t*)(&output_data[i*BUFFER_SIZE]), BUFFER_SIZE*4);
            // }

            // is_founded = compareHash((uint8_t*)(&output_data[i*BUFFER_SIZE]), target_hash, MD5_HASH_LEN);
            is_founded = compareHash((uint8_t*)(&output_data[i*BUFFER_SIZE]), target_hash, size_data[0]);
            if (is_founded) {
                printf("result: %d\n", i + iteration*num_data);
                end_time = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                printf("Hash found successfully in : %d ms\n", duration.count());
                break;
            }

        }
        iteration++;
    }
    
    // Очистка
    clReleaseKernel(kernel_sha512);
    clReleaseKernel(kernel_md5);
    clReleaseProgram(md5_lib);
    clReleaseProgram(platform_lib);
    clReleaseProgram(common_lib);
    clReleaseProgram(main_program);
    clReleaseProgram(final_program);
    clReleaseMemObject(buf_input);
    clReleaseMemObject(buf_output);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}
