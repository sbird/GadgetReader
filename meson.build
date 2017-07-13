#S-GenIC is Simeon Bird's modifications to N-GenIC, in C++.
project('GadgetReader', ['cpp','c'])

#External dependencies:
#HDF5 is pkg-config on debian based systems, but just a library elsewhere
hdf5 = [dependency('hdf5', required: false),]
if not hdf5[0].found()
    comp = meson.get_compiler('cpp')
    hdf5 = [comp.find_library('hdf5_hl', required: false),comp.find_library('hdf5', required: false)]
endif
if hdf5[0].found()
    add_project_arguments('-DHAVE_HDF5',language: 'cpp')
endif
#Add bigfile. It is unlikely to be packaged so use the internal version
add_project_arguments('-DHAVE_BGFL',language: 'cpp')
bfsrc = ['bigfile/src/bigfile.c']
bfinc = include_directories('bigfile/src')
bigfile = static_library('bigfile', sources: bfsrc)

wsrc = ['gadgetwriter.cpp', 'gadgetwritehdf.cpp', 'gadgetwriteoldgadget.cpp', 'gadgetwritebigfile.cpp']
rsrc = ['gadgetreader.cpp']
#Define output libraries
librgad = library('rgad', sources: rsrc)
libwgad = library('wgad', sources: wsrc, dependencies: hdf5, include_directories : bfinc, link_with: bigfile)
#Define utility programs
executable('Convert2HDF5',sources: 'Convert2HDF5.cpp', link_with: [librgad, libwgad])
executable('PosDump',sources: 'PosDump.cpp', link_with: librgad)
pgii = executable('PGIIhead',sources: 'PGIIhead.cpp', link_with: librgad)
#Define tests
testdep = [dependency('boost', modules: 'test'),]
testsrc = ['btest.cpp']
boostdep = [dependency('boost', modules: 'test'),]
testexe = executable('btest', sources: testsrc, link_with: librgad, dependencies: boostdep)
test('Boost Tests',testexe, workdir: meson.source_root())
#pgtest = executable('PGtest', 'PGIItest.sh', pgii)
#pgtest = executable('PGtest', 'PGIItest.sh')
#pgtest = run_target('PGtest', command : ['PGIItest.sh',pgii], build_by_default: true)
pgdata = [meson.source_root()+'/test_g2_snap', meson.source_root()+'/PGIIhead_out.txt']
test('Header test',find_program('PGIItest.sh'), args: [pgii.full_path(), ]+ pgdata)

#doc = run_target('Doxygen'
