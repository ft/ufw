if(__UFW_FreeRTOS)
  return()
endif()
set(__UFW_FreeRTOS 1)

function(use_freertos)
  cmake_parse_arguments(PA "" "TARGET;PORTABLE;HEAP" "" ${ARGN})
  if (NOT PA_PORTABLE)
    message(FATAL_ERROR "use_freertos: Supply PORTABLE argument!")
  endif()
  if (NOT PA_TARGET)
    message(FATAL_ERROR "use_freertos: Supply TARGET argument!")
  endif()
  set(portable_root ${__UFW_ROOT_FREERTOS_KERNEL}/portable/${PA_PORTABLE})
  set(kernel_files
    ${__UFW_ROOT_FREERTOS_KERNEL}/croutine.c
    ${__UFW_ROOT_FREERTOS_KERNEL}/event_groups.c
    ${__UFW_ROOT_FREERTOS_KERNEL}/list.c
    ${__UFW_ROOT_FREERTOS_KERNEL}/queue.c
    ${__UFW_ROOT_FREERTOS_KERNEL}/stream_buffer.c
    ${__UFW_ROOT_FREERTOS_KERNEL}/tasks.c
    ${__UFW_ROOT_FREERTOS_KERNEL}/timers.c)
  file(GLOB portable_files
    LIST_DIRECTORIES false
    "${portable_root}/*.c"
    "${portable_root}/*.asm"
    "${portable_root}/*.s")
  target_sources(${PA_TARGET} PUBLIC ${kernel_files} ${portable_files})
  if (PA_HEAP)
    target_sources(${PA_TARGET} PUBLIC
      ${__UFW_ROOT_FREERTOS_KERNEL}/portable/MemMang/${PA_HEAP})
  endif()
  target_include_directories(${PA_TARGET} PUBLIC
    ${__UFW_ROOT_FREERTOS_KERNEL}/include
    ${portable_root})
endfunction()

function(use_freertos_tcp)
  cmake_parse_arguments(PA "" "TARGET;PORTABLE;ALLOCATION" "" ${ARGN})
  if (NOT PA_PORTABLE)
    message(FATAL_ERROR "use_freertos_tcp: Supply PORTABLE argument!")
  endif()
  if (NOT PA_TARGET)
    message(FATAL_ERROR "use_freertos_tcp: Supply TARGET argument!")
  endif()
  if (NOT PA_ALLOCATION)
    set(PA_ALLOCATION dynamic)
  endif()
  set(portable_root ${__UFW_ROOT_FREERTOS_TCP}/portable/Compiler/${PA_PORTABLE})
  if (${PA_ALLOCATION} STREQUAL dynamic)
    set(alloc_files
      ${__UFW_ROOT_FREERTOS_TCP}/portable/BufferManagement/BufferAllocation_2.c)
  else()
    set(alloc_files
      ${__UFW_ROOT_FREERTOS_TCP}/portable/BufferManagement/BufferAllocation_1.c)
  endif()

  set(addon_files
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_ARP.c
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_DHCP.c
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_DNS.c
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_IP.c
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_Sockets.c
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_Stream_Buffer.c
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_TCP_IP.c
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_TCP_WIN.c
    ${__UFW_ROOT_FREERTOS_TCP}/FreeRTOS_UDP_IP.c)

  file(GLOB portable_files
    LIST_DIRECTORIES false
    "${portable_root}/*.c"
    "${portable_root}/*.asm"
    "${portable_root}/*.s")

  target_sources(${PA_TARGET} PUBLIC
    ${addon_files}
    ${alloc_files}
    ${portable_files})

  target_include_directories(${PA_TARGET} PUBLIC
    ${__UFW_ROOT_FREERTOS_TCP}/include
    ${portable_root})
endfunction()

function(add_freertos root)
  set(__UFW_ROOT_FREERTOS_SYSTEM ${root} PARENT_SCOPE)
  set(__UFW_ROOT_FREERTOS_KERNEL ${root}/FreeRTOS/Source PARENT_SCOPE)
endfunction()

function(add_freertos_kernel root)
  set(__UFW_ROOT_FREERTOS_KERNEL ${root} PARENT_SCOPE)
endfunction()

function(add_freertos_tcp root)
  if (NOT __UFW_ROOT_FREERTOS_SYSTEM)
    message(FATAL_ERROR
      "FreeRTOS TCP requires FreeRTOS System via add_freertos!")
  endif()
  set(__UFW_ROOT_FREERTOS_TCP ${root} PARENT_SCOPE)
endfunction()
