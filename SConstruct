import os;
VariantDir('build/src', './cxx');
env = Environment();

env.Replace(CC='xcrun clang')
env.Replace(CXX='xcrun clang++')
env.Append(CXXFLAGS=['-std=c++11', '-stdlib=libc++', '-g', '-O0', '-Wall', '-Ibuild/src/include'])
env.Append(LINKFLAGS=['-std=c++11', '-stdlib=libc++', '-lcppa']);

#env.Append(LINKFLAGS='-framework OpenAL');

srcFiles = [
	'build/src/include/komm/global.h',
	'build/src/include/komm/logic.h',
	'build/src/include/komm/sock.h',
	'build/src/source/global_impl.cpp',
	'build/src/source/sock_impl.cpp',
	'build/src/source/main.cpp',
]

env.Program(target='build/bin/fcgi', source=srcFiles);