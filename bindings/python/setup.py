
import sys
from distutils.core import setup, Extension

if sys.version_info[0] == 2:
	ver_define = ('PYTHON2', '1')
elif sys.version_info[0] == 3:
	ver_define = ('PYTHON3', '1')

fiu_ll = Extension("fiu_ll",
		sources = ['fiu_ll.c'],
		define_macros = [ver_define],
		libraries = ['fiu'],

		# these two allow us to build without having libfiu installed,
		# assuming we're in the libfiu source tree
		include_dirs = ['../../libfiu/'],
		library_dirs=['../../libfiu/'])

setup(
	name = 'fiu',
	description = "libfiu bindings",
	author = "Alberto Bertogli",
	author_email = "albertito@blitiri.com.ar",
	url = "http://blitiri.com.ar/p/libfiu",
	py_modules = ['fiu'],
	ext_modules = [fiu_ll]
)

