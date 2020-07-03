# -*- coding: utf-8 -*-
def main():  # nocover
    import pydarknet

    print('Looks like the imports worked')
    print('pydarknet = {!r}'.format(pydarknet))
    print('pydarknet.__file__ = {!r}'.format(pydarknet.__file__))
    print('pydarknet.__version__ = {!r}'.format(pydarknet.__version__))
    print('pydarknet.DARKNET_CLIB = {!r}'.format(pydarknet.DARKNET_CLIB))


if __name__ == '__main__':
    """
    CommandLine:
       python -m pydarknet
    """
    main()
