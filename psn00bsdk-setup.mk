# PSn00bSDK project setup file
# Part of the PSn00bSDK Project
# 2019 - 2020 Lameguy64 / Meido-Tek Productions
#
# This file may be copied for use with your projects, see the template
# directory for a makefile template

ifndef PREFIX

PREFIX		= mipsel-unknown-elf

endif	# PREFIX

ifndef GCC_VERSION

GCC_VERSION	= 7.4.0

endif	# GCC_VERSION

# PSn00bSDK library/include path setup
ifndef PSN00BSDK_LIBS

# Default assumes PSn00bSDK is in the same parent dir as this project

LIBDIRS		= -L../psn00bsdk/libpsn00b
INCLUDE	 	= -I../psn00bsdk/libpsn00b/include

else

LIBDIRS		= -L$(PSN00BSDK_LIBS)
INCLUDE		= -I$(PSN00BSDK_LIBS)/include

endif 		# PSN00BSDK_LIBS

# PSn00bSDK toolchain path setup
ifndef GCC_BASE

ifndef PSN00BSDK_TC

# Default assumes GCC toolchain is in root of C drive or /usr/local

ifeq "$(OS)" "Windows_NT"

GCC_BASE	= /c/mipsel-unknown-elf
GCC_BIN		=

else

GCC_BASE	= /usr/local/mipsel-unknown-elf
GCC_BIN		=

endif

else

GCC_BASE	= $(PSN00BSDK_TC)
GCC_BIN		= $(PSN00BSDK_TC)/bin/

endif		# PSN00BSDK_TC

endif		# GCC_BASE

CC			= $(GCC_BIN)$(PREFIX)-gcc
CXX			= $(GCC_BIN)$(PREFIX)-g++
AS			= $(GCC_BIN)$(PREFIX)-as
AR			= $(GCC_BIN)$(PREFIX)-ar
LD			= $(GCC_BIN)$(PREFIX)-ld
RANLIB		= $(GCC_BIN)$(PREFIX)-ranlib
