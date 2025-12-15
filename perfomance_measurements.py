import matplotlib.pyplot as plt

degrees = [3, 4, 5, 6, 7, 8, 9]
opencl = [1.336, 1.089, 1.154, 1.086, 2.244, 13.269, 129.414]
hashlib = [0.03, 0.07, 0.4, 3.64, 36.02, 358.79, 3444.9]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 8))

ax1.plot(degrees, opencl, 'bp--', label='OpenCL', linewidth=2, markersize=8)
ax1.plot(degrees, hashlib, 'rp--', label='Python hashlib', linewidth=2, markersize=8)
ax1.set_title('`OpenCL` vs `Python hashlib`')
ax1.set_xlabel('Степени числа 10')
ax1.set_ylabel('Время выполнения, с')
ax1.grid(True)
ax1.legend()

ax2.plot(degrees[:-2], opencl[:-2], 'bp--', label='OpenCL', linewidth=2, markersize=8)
ax2.plot(degrees[:-2], hashlib[:-2], 'rp--', label='Python hashlib', linewidth=2, markersize=8)
ax2.set_title('`OpenCL` vs `Python hashlib` (до 7 степени)')
ax2.set_xlabel('Степени числа 10')
ax2.grid(True)
ax2.legend()

plt.tight_layout()
plt.show()