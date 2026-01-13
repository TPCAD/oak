UEFI（Unified Extensible Firmware Interface），统一可扩展固件接口，是一套为系统平台固件所制定的**标准**。UEFI 标准定义了 PC 操作系统和平台固件之间的一系列软件接口，这些接口为引导操作系统提供了一个标准化的环境。

UEFI 的前身是 Intel 的 EFI，它是 Intel 为了取代 BIOS 而制定的一套标准，最初名为 Intel Boot Initiative，随后更名为 Extensible Firmwre Interface。2005 年 7 月，Intel 停止了 EFI 标准的开发，并把它贡献给了 Unified EFI Forum。

UEFI 的目标是取代 BIOS，为引导操作系统提供标准化环境。但 UEFI 与 BIOS 并不冲突，不管是传统主板或支持 UEFI 的主板都包含 BIOS ROM。不同的只是它们如何寻找 bootloader 或内核，如何加载内核以及它们提供的接口。

UEFI 支持 32 位或更高的处理器架构，但只支持小端序的处理器。
