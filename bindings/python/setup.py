from setuptools import setup, find_packages

setup(name = "pycpslib",
      version = "0.1",
      packages = find_packages(),
      setup_requires=["cffi>=1.0.0"],
      cffi_modules=["pycpslib_build.py:ffi"],
      install_requires = ["cffi>=1.0.0"],
      author = "Noufal Ibrahim",
      author_email = "noufal@nibrahim.net.in",
      description = "Python bindings for the cpslib library",
      license = "MIT",
      keywords = "tests",
      url = "http://github.com/nibrahim/cpslib",
      )

      
