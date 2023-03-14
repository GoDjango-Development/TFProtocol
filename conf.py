# This is for sphinx use
# conf.py

import os
import sys

sys.path.insert(0, os.path.abspath('.'))

extensions = ['recommonmark']

source_suffix = ['.rst', '.md']

master_doc = 'README'

project = 'TFProtocol'

author = 'Esteban Chacon Martin, Luis Miguel Arias'

html_theme = 'alabaster'

exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', ".*"]

# Add any other configuration settings you need here
