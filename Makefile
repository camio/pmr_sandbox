.PHONY: all clean

# BDE_INCLUDES:=-I/opt/bb/include
# BDE_LIBS:=-L/opt/bb/lib64 -lbdl -lbsl

EXECUTABLES:= \
  simplicity.gcc \
  simplicity.clang \
  libcpp_bug.gcc \
  libcpp_bug.clang \
  before_after.gcc \
  before_after.clang

all: ${EXECUTABLES}

simplicity.gcc: simplicity.cpp
	g++ -std=c++17 -I. ${BDE_INCLUDES} $< ${BDE_LIBS} -o $@

simplicity.clang: simplicity.cpp
	clang++ -std=c++17 -stdlib=libc++ -I. ${BDE_INCLUDES} $< -lc++experimental ${BDE_LIBS} -o $@

libcpp_bug.gcc: libcpp_bug.cpp
	g++ -std=c++17 -I. ${BDE_INCLUDES} $< ${BDE_LIBS} -o $@

libcpp_bug.clang: libcpp_bug.cpp
	clang++ -std=c++17 -stdlib=libc++ -I. ${BDE_INCLUDES} $< -lc++experimental ${BDE_LIBS} -o $@

before_after.gcc: before_after.cpp
	g++ -std=c++17 -I. ${BDE_INCLUDES} $< ${BDE_LIBS} -o $@

before_after.clang: before_after.cpp
	clang++ -std=c++17 -stdlib=libc++ -I. ${BDE_INCLUDES} $< -lc++experimental ${BDE_LIBS} -o $@

clean:
	$(RM) ${EXECUTABLES}
