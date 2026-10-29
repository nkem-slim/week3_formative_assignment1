from setuptools import Extension, setup


setup(
    name="data_analysis",
    version="1.0",
    ext_modules=[
        Extension("data_analysis", sources=["data_analysis.c"], libraries=["m"])
    ],
)
