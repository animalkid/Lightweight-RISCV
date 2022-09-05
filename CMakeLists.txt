cmake_minimum_required(VERSION 3.12)
set(CMAKE_TOOLCHAIN_FILE KendryteToolchain.cmake) # Must be set before project() command
project(kendryte-freertos-project C ASM) # Set project name here. PROJECT_NAME gets set from this.

add_compile_definitions(
	CONFIG_LOG_LEVEL=LOG_VERBOSE
	CONFIG_LOG_ENABLE
	CONFIG_LOG_COLORS
	LOG_KERNEL
	__riscv64
	LV_CONF_INCLUDE_SIMPLE
)

include_directories(kendryte-standalone-sdk/lib/utils/include)
add_subdirectory(kendryte-standalone-sdk/lib)

#message("ALG=${ALG}\n")

if (${TYPE} STREQUAL "aead" OR ${TYPE} STREQUAL "hash" OR ${TYPE} STREQUAL "prf")
add_executable(${PROJECT_NAME}
	FreeRTOS-Kernel/tasks.c
	FreeRTOS-Kernel/queue.c
	FreeRTOS-Kernel/list.c
	FreeRTOS-Kernel/portable/GCC/RISC-V/port.c
	FreeRTOS-Kernel/portable/GCC/RISC-V/portASM.S
	FreeRTOS-Kernel/portable/MemMang/heap_4.c
	port/ExternalInterruptHandler.c

	src/main.c
	#src/led.c
	src/lcd.c
	src/nt35310.c
	src/cypher/ascon/ascon-lib/printstate.c
	src/cypher/ascon/algorithms/${ALG}/${TYPE}.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
	FreeRTOS-Kernel/include
	.
	FreeRTOS-Kernel/portable/GCC/RISC-V/
	port
	kendryte-standalone-sdk/lib/
	kendryte-standalone-sdk/lib/drivers/include
	kendryte-standalone-sdk/lib/bsp/include
	src
	src/lib
	src/cypher/ascon/ascon-lib
	src/cypher/ascon/algorithms/${ALG}
)

elseif(${TYPE} STREQUAL "sha256")
add_executable(${PROJECT_NAME}
	FreeRTOS-Kernel/tasks.c
	FreeRTOS-Kernel/queue.c
	FreeRTOS-Kernel/list.c
	FreeRTOS-Kernel/portable/GCC/RISC-V/port.c
	FreeRTOS-Kernel/portable/GCC/RISC-V/portASM.S
	FreeRTOS-Kernel/portable/MemMang/heap_4.c
	port/ExternalInterruptHandler.c

	src/main.c
	#src/led.c
	src/lcd.c
	src/nt35310.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
	FreeRTOS-Kernel/include
	.
	FreeRTOS-Kernel/portable/GCC/RISC-V/
	port
	kendryte-standalone-sdk/lib/
	kendryte-standalone-sdk/lib/drivers/include
	kendryte-standalone-sdk/lib/bsp/include
	src
	src/lib
	src/cypher/${TYPE}
)

elseif(${TYPE} STREQUAL "aes_software")
add_executable(${PROJECT_NAME}
	FreeRTOS-Kernel/tasks.c
	FreeRTOS-Kernel/queue.c
	FreeRTOS-Kernel/list.c
	FreeRTOS-Kernel/portable/GCC/RISC-V/port.c
	FreeRTOS-Kernel/portable/GCC/RISC-V/portASM.S
	FreeRTOS-Kernel/portable/MemMang/heap_4.c
	port/ExternalInterruptHandler.c

	src/main.c
	#src/led.c
	src/lcd.c
	src/nt35310.c
	src/cypher/aes_software/aes_soft.c

)

target_include_directories(${PROJECT_NAME} PRIVATE
	FreeRTOS-Kernel/include
	.
	FreeRTOS-Kernel/portable/GCC/RISC-V/
	port
	kendryte-standalone-sdk/lib/
	kendryte-standalone-sdk/lib/drivers/include
	kendryte-standalone-sdk/lib/bsp/include
	src
	src/lib
	#src/cypher/${TYPE}
	src/cypher/aes_software/${ALG}/${KEY}
)

else ()
add_executable(${PROJECT_NAME}
	FreeRTOS-Kernel/tasks.c
	FreeRTOS-Kernel/queue.c
	FreeRTOS-Kernel/list.c
	FreeRTOS-Kernel/portable/GCC/RISC-V/port.c
	FreeRTOS-Kernel/portable/GCC/RISC-V/portASM.S
	FreeRTOS-Kernel/portable/MemMang/heap_4.c
	port/ExternalInterruptHandler.c

	src/main.c
	#src/led.c
	src/lcd.c
	src/nt35310.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
	FreeRTOS-Kernel/include
	.
	FreeRTOS-Kernel/portable/GCC/RISC-V/
	port
	kendryte-standalone-sdk/lib/
	kendryte-standalone-sdk/lib/drivers/include
	kendryte-standalone-sdk/lib/bsp/include
	src
	src/lib
	src/cypher/${TYPE}/${ALG}/${KEY}
)

endif ()



target_link_libraries(${PROJECT_NAME}
	-Wl,--start-group
	gcc m c
	-Wl,--whole-archive
	kendryte
	-Wl,--no-whole-archive
	-Wl,--end-group
)
target_link_options(${PROJECT_NAME} PRIVATE -T "${CMAKE_CURRENT_SOURCE_DIR}/kendryte-standalone-sdk/lds/kendryte.ld")
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX .elf)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_OBJCOPY}
		--output-format=binary
		${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
		${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
	COMMENT "Generating .bin file"
)
