
from distutils.core import setup, Extension

fiu_ll = Extension("fiu_ll",
		libraries = ['fiu'],
		sources = ['fiu_ll.c'])

setup(
	name = 'fiu',
	description = "libfiu bindings",
	author = "Alberto Bertogli",
	author_email = "albertito@blitiri.com.ar",
	url = "http://blitiri.com.ar/p/libfiu",
	py_modules = ['fiu'],
	ext_modules = [fiu_ll]
)

