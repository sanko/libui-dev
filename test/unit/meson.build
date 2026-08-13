# the vendored cmocka (2.0.2) self-reports version 1.1.5
cmocka_deps = dependency('cmocka', version: '>=1.1.5',
	fallback : ['cmocka', 'cmocka_dep'], default_options: ['default_library=static'])

libui_unit_sources = [
	'main.c',
	'init.c',
	'slider.c',
        'spinbox.c',
        'label.c',
        'button.c',
        'combobox.c',
        'editablecombobox.c',
        'checkbox.c',
        'radiobuttons.c',
        'tab.c',
        'scroll.c',
        'entry.c',
        'menu.c',
        'progressbar.c',
        'drawmatrix.c',
        'image.c',
        'imagebuffer.c',
        'attrstr.c',
        'window.c',
        'tooltip.c',
        'dragdestination.c',
]

if libui_OS == 'windows'
	libui_unit_manifest = 'unit.manifest'
	if libui_mode == 'static'
		libui_unit_manifest = 'unit.static.manifest'
	endif
	libui_unit_sources += [
		windows.compile_resources('resources.rc',
			args: libui_manifest_args,
			depend_files: [libui_unit_manifest]),
	]
elif libui_OS == 'darwin'
	libui_unit_sources += ['dragdestination_darwin.m']
else
	libui_unit_sources += ['dragdestination_unix.c']
endif

unit = executable('unit', libui_unit_sources,
	dependencies: [libui_binary_deps, cmocka_deps],
	link_with: libui_libui,
	win_subsystem: 'console',
	install: false)

test('Unit Tests', unit)
