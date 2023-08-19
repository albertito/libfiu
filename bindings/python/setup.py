import os
import sys
import tempfile
from setuptools import setup, Extension
from setuptools.command.build_py import build_py


# We need to generate the fiu_ctrl.py file from fiu_ctrl.in.py, replacing some
# environment variables in its contents.
class generate_and_build_py(build_py):
    def run(self):
        if not self.dry_run:
            self.generate_fiu_ctrl()
        build_py.run(self)

    def generate_fiu_ctrl(self):
        prefix = os.environ.get("PREFIX", "/usr/local/")
        plibpath = os.environ.get("PLIBPATH", prefix + "/lib/")

        contents = open("fiu_ctrl.in.py", "rt").read()
        contents = contents.replace("@@PLIBPATH@@", plibpath)

        # Create/update the file atomically, we don't want to accidentally use
        # partially written files.
        out = tempfile.NamedTemporaryFile(
            mode="wt", delete=False, dir=".", prefix="tmp-fiu_ctrl.py"
        )
        out.write(contents)
        out.close()
        os.rename(out.name, "fiu_ctrl.py")


fiu_ll = Extension(
    "fiu_ll",
    sources=["fiu_ll.c"],
    libraries=["fiu"],
    # These two allow us to build without having libfiu installed,
    # assuming we're in the libfiu source tree
    include_dirs=["../../libfiu/"],
    library_dirs=["../../libfiu/"],
)

setup(
    name="fiu",
    description="libfiu bindings",
    author="Alberto Bertogli",
    author_email="albertito@blitiri.com.ar",
    url="http://blitiri.com.ar/p/libfiu",
    py_modules=["fiu", "fiu_ctrl"],
    ext_modules=[fiu_ll],
    cmdclass={"build_py": generate_and_build_py},
)
