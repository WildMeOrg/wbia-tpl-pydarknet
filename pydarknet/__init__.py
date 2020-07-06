# -*- coding: utf-8 -*-
# flake8: noqa
from __future__ import absolute_import, division, print_function

try:
    from pydarknet._version import __version__
except ImportError:
    __version__ = '0.0.0'

import utool as ut

ut.noinject(__name__, '[pydarknet.__init__]')

from ._pydarknet import *
