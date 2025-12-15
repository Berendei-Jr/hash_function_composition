#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

// Функция для проверки ошибок OpenCL
void check_error(cl_int err, const char* operation) {
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error during %s: %d\n", operation, err);
        exit(1);
    }
}

// Функция для вывода типа устройства
const char* get_device_type(cl_device_type type) {
    switch (type) {
        case CL_DEVICE_TYPE_GPU: return "GPU";
        case CL_DEVICE_TYPE_CPU: return "CPU";
        case CL_DEVICE_TYPE_ACCELERATOR: return "Accelerator";
        case CL_DEVICE_TYPE_DEFAULT: return "Default";
        default: return "Unknown";
    }
}

// Функция для вывода размера в читаемом формате
void print_size(size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit_index = 0;
    double size_d = (double)size;
    
    while (size_d >= 1024.0 && unit_index < 3) {
        size_d /= 1024.0;
        unit_index++;
    }
    printf("%.2f %s", size_d, units[unit_index]);
}

int main() {
    cl_platform_id platform;
    cl_device_id device;
    cl_int err;
    
    // Получаем первую платформу
    err = clGetPlatformIDs(1, &platform, NULL);
    check_error(err, "getting platform IDs");
    
    // Получаем первое GPU устройство
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err == CL_DEVICE_NOT_FOUND) {
        printf("GPU not found, trying CPU...\n");
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
    }
    check_error(err, "getting device IDs");
    
    // Получаем и выводим информацию об устройстве
    printf("=== OpenCL Device Capabilities ===\n\n");
    
    // Имя устройства
    char device_name[128];
    err = clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    check_error(err, "getting device name");
    printf("Device Name: %s\n", device_name);
    
    // Тип устройства
    cl_device_type device_type;
    err = clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);
    check_error(err, "getting device type");
    printf("Device Type: %s\n", get_device_type(device_type));
    
    // Версия OpenCL
    char opencl_version[128];
    err = clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(opencl_version), opencl_version, NULL);
    check_error(err, "getting OpenCL version");
    printf("OpenCL Version: %s\n", opencl_version);
    
    // Количество вычислительных модулей
    cl_uint compute_units;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
    check_error(err, "getting compute units");
    printf("Max Compute Units: %u\n", compute_units);
    
    // Максимальный размер рабочей группы
    size_t max_work_group_size;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);
    check_error(err, "getting max work group size");
    printf("Max Work-Group Size: %zu work-items\n", max_work_group_size);
    
    // Размерности рабочей группы
    cl_uint max_work_item_dimensions;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(max_work_item_dimensions), &max_work_item_dimensions, NULL);
    check_error(err, "getting max work item dimensions");
    
    size_t* max_work_item_sizes = (size_t*)malloc(max_work_item_dimensions * sizeof(size_t));
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, max_work_item_dimensions * sizeof(size_t), max_work_item_sizes, NULL);
    check_error(err, "getting max work item sizes");
    
    printf("Max Work-Item Dimensions: %u\n", max_work_item_dimensions);
    printf("Max Work-Item Sizes: ");
    for (cl_uint i = 0; i < max_work_item_dimensions; i++) {
        printf("%zu", max_work_item_sizes[i]);
        if (i < max_work_item_dimensions - 1) printf(" x ");
    }
    printf("\n");
    free(max_work_item_sizes);
    
    // Локальная память
    cl_ulong local_mem_size;
    err = clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem_size), &local_mem_size, NULL);
    check_error(err, "getting local memory size");
    printf("Local Memory Size: ");
    print_size(local_mem_size);
    printf("\n");
    
    // Глобальная память
    cl_ulong global_mem_size;
    err = clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);
    check_error(err, "getting global memory size");
    printf("Global Memory Size: ");
    print_size(global_mem_size);
    printf("\n");
    
    // Константная память
    cl_ulong constant_mem_size;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(constant_mem_size), &constant_mem_size, NULL);
    check_error(err, "getting constant buffer size");
    printf("Max Constant Buffer Size: ");
    print_size(constant_mem_size);
    printf("\n");
    
    // Тактовая частота
    cl_uint clock_frequency;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);
    check_error(err, "getting clock frequency");
    printf("Max Clock Frequency: %u MHz\n", clock_frequency);
    
    // Размер кэша
    cl_ulong cache_size;
    err = clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cache_size), &cache_size, NULL);
    check_error(err, "getting cache size");
    printf("Global Memory Cache Size: ");
    print_size(cache_size);
    printf("\n");
    
    // Максимальное выделение памяти
    cl_ulong max_alloc_size;
    err = clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(max_alloc_size), &max_alloc_size, NULL);
    check_error(err, "getting max memory allocation size");
    printf("Max Memory Allocation Size: ");
    print_size(max_alloc_size);
    printf("\n");
    
    // Размерность выравнивания
    cl_uint mem_base_addr_align;
    err = clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(mem_base_addr_align), &mem_base_addr_align, NULL);
    check_error(err, "getting memory base address align");
    printf("Memory Base Address Align: %u bits\n", mem_base_addr_align);
    
    // Доступность устройства
    cl_bool available;
    err = clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, sizeof(available), &available, NULL);
    check_error(err, "getting device availability");
    printf("Device Available: %s\n", available ? "Yes" : "No");
    
    printf("\n=== End of Device Capabilities ===\n");
    
    return 0;
}