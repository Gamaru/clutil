`void Device::SetCurrentDevice(size_t deviceNum)`

Sets the current device for subsequent allocation and kernel enqueue operations. The `deviceNum` parameter denotes the cardinal number of the desired device. This modifies the global state internal to the [Device](Device.md) class. Unless otherwise specified, subsequent instantiations of a [Buffer](Buffer.md) or [Image](Image.md) object or enqueueing a kernel will occur the deviceNum<sup>th</sup> device after this call.

# Return #

  * None

# Parameters #

  * deviceNum: the cardinal number of the desired device.