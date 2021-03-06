.SECONDEXPANSION:

# some cleaning up to do

# compiler detection

ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
clang?=1
else
clang?=0
endif

# default target

ifeq ($(OS),Windows_NT)
target?=win32
else
target?=xlib
endif

# default executables

ifeq ($(target),win32)
ifeq ($(OS),Windows_NT)
windres?=windres
else
windres?=x86_64-w64-mingw32-windres
endif
endif

python ?= python3
glsl ?= glslangValidator

#

outdir ?= bin/

#

flags = -Wall -Wextra -std=gnu11 -fms-extensions -Ishared/ -iquote$(path) -iquotetmp/ \
-Duse_result="__attribute__((warn_unused_result))" \
-Dcountof\(x\)="(sizeof(x)/sizeof(*(x)))" \
$(CFLAGS)

ifeq ($(clang),1)
flags += -Wno-initializer-overrides
else
flags += -Wno-override-init
endif

ifeq ($(fast),1)
flags+=\
-Dforce_inline="__attribute__((always_inline))" -Wno-attributes \
-Ofast -flto -DNDEBUG -U_FORTIFY_SOURCE -fno-stack-protector \
-fomit-frame-pointer -fmerge-all-constants  \
-fassociative-math -freciprocal-math -fno-signed-zeros -fno-trapping-math \
-fivopts -fweb -frename-registers \
--param max-inline-insns-single=999999999 --param max-inline-insns-auto=999999999 \
--param large-function-insns=999999999 --param early-inlining-insns=999999999 \
--param inline-unit-growth=10000000 --param max-early-inliner-iterations=999999999 \
--param large-stack-frame=100000000 --param large-stack-frame-growth=10000 \
--param ipcp-unit-growth=10000 --param min-inline-recursive-probability=0 \
--param gcse-unrestricted-cost=0 --param max-hoist-depth=0 \
-fsingle-precision-constant \
-Wl,--build-id=none -fno-asynchronous-unwind-tables -s

ifeq ($(clang),1)
flags +=
else
flags += -flto-partition=one -Wtrampolines -Wunsafe-loop-optimizations -funsafe-loop-optimizations
endif

#-Qn
#-fdata-sections -ffunction-sections -Wl,--gc-sections
#-fno-partial-inlining
#small functions marked as cold by gcc still arent inlined...
else
flags+=-g
endif

shared=net util
src=$(wildcard $(path)/*.c) $(wildcard $(path)/$(target)/*.c) $(addsuffix .c,$(addprefix shared/,$(sort $(shared))))
dep=$(wildcard $(path)/*.h) $(wildcard $(path)/$(target)/*.h) $(addsuffix .h,$(addprefix shared/,$(sort $(shared))))

wflags=-D_WIN32_WINNT=0x600 -lws2_32 -lcomctl32 -lgdi32
xflags=

out ?= $(outdir)$(path)
ifeq ($(target),win32)
flags += $(wflags)
ext ?= .exe
else
flags += $(xflags)
endif

client : path = client
client : src += $(wildcard $(path)/audio/*.c) $(wildcard $(path)/text/*.c) $(wildcard shared/xz/*.c)
client : dep += $(wildcard $(path)/audio/*.h) $(wildcard $(path)/text/*.h) $(wildcard shared/xz/*.h)
client : xflags += -lX11 -lopus -lasound -pthread -lm
client : wflags += -lws2_32 -lgdi32 -lole32 -DNAME="L\"warcog\""

ifeq ($(vulkan),1)
client : flags += -DUSE_VULKAN -lvulkan
client : xflags += -DVK_USE_PLATFORM_XCB_KHR -lX11-xcb
client : wflags += -DVK_USE_PLATFORM_WIN32_KHR
client : dep += $(outdir)shaders-vk tmp/shaders-vk.h
else
client : xflags += -lGL
client : wflags += -lopengl32
client : dep += $(outdir)shaders-gl tmp/shaders-gl.h
endif

chatclient : path = chatclient
chatclient : src += tmp/res.res

server : path = server
server : fps ?= 60
server : port ?= 1337
server : src += tmp/text.c tmp/gen.c $(wildcard $(map)/*.c)
server : dep += tmp/text.h tmp/gen.h $(wildcard $(map)/*.h) $(outdir)datamap $(outdir)data tmp/data.h
server : flags += -iquote$(map)/ -lm

chatserver : path = chatserver
chatserver : fps ?= 2
chatserver : port ?= 1336

server chatserver : shared += ip
server chatserver : src += shared/server.c
server chatserver : flags += -DFPS=$(fps) -DPORT=$(port)

client chatclient server chatserver : tmp/ $(outdir) $$(src) $$(dep)
	$(CC) $(libs) $(src) $(flags) -o $(addsuffix $(ext),$(out))

website : $(outdir) $(wildcard website/*)
	$(python) website/md_to_html.py $(outdir) $(wildcard website/*.md)

$(outdir) :
	mkdir $(outdir)

tmp/ :
	mkdir tmp

#client

$(outdir)shaders-gl tmp/shaders-gl.h : client/tools/shaders-gl.py $(wildcard client/shaders-gl/*)
	$(python) client/tools/shaders-gl.py client/shaders-gl/ $(outdir)shaders-gl tmp/shaders-gl.h

$(outdir)shaders-vk tmp/shaders-vk.h : client/tools/shaders-vk.py $(wildcard client/shaders-vk/*)
	$(python) client/tools/shaders-vk.py $(glsl) client/shaders-vk/ $(outdir)shaders-vk tmp/shaders-vk.h

#server

tmp/text.c tmp/text.h : $(map)/text
	$(python) server/tools/maketext.py $(map)/text tmp/

tmp/gen.c tmp/gen.h tmp/functions.h : server/tools/gen.py $(wildcard $(map)/src/*)
	$(python) server/tools/gen.py tmp/ $(map) $(wildcard $(map)/src/*)

tmp/defs : $(wildcard $(map)/*.h) server/defstruct.h tmp/gen.c $(map)/tilemap
	$(python) server/tools/makedata0.py $(map)/tilemap tmp/data.h
	$(CC) server/tools/defs.c tmp/gen.c tmp/text.c $(flags) -DBUILD_DEFS -o tmp/defs

tmp/defdata : tmp/defs
	tmp/defs tmp/defdata

$(outdir)datamap $(outdir)data tmp/data.h : server/tools/makedata.py tmp/defdata
	$(python) server/tools/makedata.py $(map)/tilemap tmp/defdata $(outdir) tmp/data.h

#chatclient

tmp/res.res : chatclient/win32/res.rc chatclient/win32/Application.manifest
	$(windres) chatclient/win32/res.rc tmp/res.res --output-format=coff

.PHONY: client server chatclient chatserver website
