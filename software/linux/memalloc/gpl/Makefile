# Copyright 2012 Google Inc. All Rights Reserved.

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DMEMALLOC_DEBUG
else
  DEBFLAGS = -O2
endif

# base address and size (MB) for linear memory allocation
HLINA_START := 0x02000000
HLINA_SIZE  := 96

EXTRA_CFLAGS += $(DEBFLAGS)
EXTRA_CFLAGS += -DHLINA_START_ADDRESS=$(HLINA_START) -DHLINA_SIZE=$(HLINA_SIZE)

ifneq ($(KERNELRELEASE),)
# recursive call from kernel build system

memalloc-objs :=
obj-m         += memalloc.o

else

KDIR := /export/Testing/Board_Version_Control/SW_Common/SOCLE_MDK-3D/openlinux/2.6.29/v0_5/android_linux-2.6.29

PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

endif

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
	rm -rf modules.order Module.symvers
