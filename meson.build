project('ocgcore', 'cpp',
	license : 'AGPL-3.0-or-later',
	default_options : ['cpp_std=c++17', 'warning_level=3', 'cpp_rtti=true', 'backend_startup_project=ocgcore'],
	version : '11.0'
)

subdir('lua')
boost_dep = dependency('boost', version : '==1.83.0')

linker_args = []
ocg_args = []
if host_machine.system() == 'linux'
    linker_args += ['-ldl']
endif
if get_option('default_library') == 'shared'
	ocg_args += ['-DOCGCORE_EXPORT_FUNCTIONS']
	if meson.get_compiler('cpp').get_id() != 'msvc'
		linker_args += ['-fvisibility=hidden']
	endif
endif

if host_machine.system() == 'darwin'
	ocg_args += ['-D_LIBCPP_NO_EXCEPTIONS']
endif

ocgcore_src = files([
    'clone.cpp',
    'clone/persistence/api.cpp',
    'clone/persistence/graph.cpp',
    'clone/persistence/objects.cpp',
    'clone/persistence/field.cpp',
    'clone/persistence/vm.cpp',
    'clone/persistence/values.cpp',
    'clone/persistence/image.cpp',

    'clone/random.cpp',
    'clone/checkpoint/api.cpp',
    'clone/checkpoint/capture.cpp',
    'clone/checkpoint/restore.cpp',
    'clone/memory/buffer.cpp',
    'clone/memory/object_pool.cpp',
    'clone/lua/heap.cpp',
    'clone/lua/region.cpp',
    'clone/lua/vm.cpp',
    'clone/graph/duel.cpp',
    'clone/graph/objects.cpp',
    'clone/graph/field.cpp',
    'clone/graph/vm.cpp',
    'clone/profiling/metrics.cpp',

	'card.cpp',
	'duel.cpp',
	'effect.cpp',
	'field.cpp',
	'interpreter.cpp',
	'libcard.cpp',
	'libdebug.cpp',
	'libduel.cpp',
	'libeffect.cpp',
	'libgroup.cpp',
	'ocgapi.cpp',
	'operations.cpp',
	'playerop.cpp',
	'processor.cpp',
	'processor_visit.cpp',
	'scriptlib.cpp',
])

ocgcore_lib = library('ocgcore', ocgcore_src, cpp_args : ocg_args, link_args : linker_args, dependencies : [lua_dep, boost_dep])
ocgcore_dep = declare_dependency(include_directories : include_directories('./'), link_with : ocgcore_lib)
