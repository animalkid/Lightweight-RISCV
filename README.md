# Lightweight cryptographic algorithms' implementation in Kendryte K210 using FreeRTOS

This project implements different lightweight cryptographic algorithms in a Kendryte K210 boards. To do so it uses a FreeRTOS port. As the official Kendryte FreeRTOS SDK was deprecated, a new port was needed, and it was taken from the following GitHub: https://github.com/cmdrf/kendryte-freertos-project

More information about the port can be found there. Also, some changes were made in the CMake, in order to build and execute the different implementations in a more efficient and family friendly way.

## Building and executing

Once the repository is cloned, the only settings to do are in the test.sh file in the build directory. Once the file is opened, go to the last lines. There will be a flashing instruction:

kflash -p /dev/ttyUSBX kendryte-freertos-project.bin -t

You will have to put the right USB port where the X is placed in order to flash the application.
 
Apart from that, there could be some problems with the CMake. If that happens you may have to check where your RISC-V toolchain was installed and change the path.
 
 -DKENDRYTE_TOOLCHAIN=/opt/riscv-toolchain ..
 
 The one above is the parameter that specifies where the toolchain is located. But it may be easier to manage to change where the toolchain is installed.
 
 Once the settings are done, giving execution permissions to the file will be the next step.
 
 Last, execute the Shell Script and answer the questions in order to build and execute the desired algorithm.
