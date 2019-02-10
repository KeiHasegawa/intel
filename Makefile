SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
INTEL_DLL = intel.dll

XX_OBJS = $(SRCS:.cpp=.obj)
INTEL_XX_DLL = intelxx.dll

DEBUG_FLAG = -g
PIC_FLAG = -fPIC
CXXFLAGS = $(DEBUG_FLAG) $(PIC_FLAG) -I$(HCC1_SRCDIR) -w
CXXFLAGS_FOR_XX = $(DEBUG_FLAG) $(PIC_FLAG) -I$(HCXX1_SRCDIR) -w -DCXX_GENERATOR

UNAME := $(shell uname)
DLL_FLAG =  -shared
ifneq (,$(findstring Darwin,$(UNAME)))
	DLL_FLAG = -dynamiclib
endif

turbo = $(if $(wildcard /etc/turbolinux-release),1,0)
ifeq ($(turbo),1)
  CXXFLAGS += -DTURBO_LINUX
  CXXFLAGS_FOR_XX += -DTURBO_LINUX
endif

RM = rm -r -f

all:$(INTEL_DLL) $(INTEL_XX_DLL)

$(INTEL_DLL) : $(OBJS)
	$(CXX) $(DEBUG_FLAG) $(PROF_FLAG) $(DLL_FLAG) -o $@ $(OBJS)

$(INTEL_XX_DLL) : $(XX_OBJS)
	$(CXX) $(DEBUG_FLAG) $(PROF_FLAG) $(DLL_FLAG) -o $@ $(XX_OBJS)

clean:
	$(RM) *.o *~ $(INTEL_DLL) .vs x64 Debug Release
	$(RM) *.obj $(INTEL_XX_DLL)

%.obj : %.cpp
	$(CXX) $(CXXFLAGS_FOR_XX) -c $< -o $@
