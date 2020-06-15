#!/usr/bin/env python
import ast
import pathlib

from setuptools import setup, find_packages


setup_requires = ()
install_requires = ()


def parse_version(fpath):
    """Statically parse the version number from a python file"""
    py_module = pathlib.Path(fpath)
    if not py_module.exists():
        raise ValueError('fpath={!r} does not exist'.format(fpath))
    with py_module.open('r') as fb:
        sourcecode = fb.read()
    pt = ast.parse(sourcecode)

    class VersionVisitor(ast.NodeVisitor):
        def visit_Assign(self, node):
            for target in node.targets:
                if getattr(target, 'id', None) == '__version__':
                    self.version = node.value.s

    visitor = VersionVisitor()
    visitor.visit(pt)
    return visitor.version


name = 'wbia-pydarknet'
version = parse_version('pydarknet/__init__.py')  # must be global for git tags

setup_kwargs = dict(
    name=name,
    version=version,
    license='YOLO',
    description=('Detects objects in images using Darknet'),
    packages=find_packages(exclude=['tests', 'tests.*']),
    url='https://github.com/WildbookOrg/wbia-tpl-pydarknet',
    author='Jason Parham, WildMe Developers',
    author_email='dev@wildme.org',
    setup_requires=setup_requires,
    install_requires=install_requires,
    # package_data={'build': util_cplat.get_dynamic_lib_globstrs()},
    cmake_args=[
        '-DCMAKE_BUILD_TYPE=Release',
        '-DCUDA=On',
    ],
)


if __name__ == '__main__':
    import skbuild
    skbuild.setup(**setup_kwargs)
