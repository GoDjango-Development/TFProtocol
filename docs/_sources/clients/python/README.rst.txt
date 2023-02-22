================================================
TFProtocol Client Implemented in Python :snake:
================================================
.. image:: https://github.com/GoDjango-Development/tfprotocol_client_py/actions/workflows/push-master.yml/badge.svg?branch=master
    :target: https://github.com/GoDjango-Development/tfprotocol_client_py/actions/workflows/push-master.yml

.. image:: https://github.com/GoDjango-Development/tfprotocol_client_py/actions/workflows/push-pull.yml/badge.svg
    :target: https://github.com/GoDjango-Development/tfprotocol_client_py/actions/workflows/push-pull.yml

.. image:: https://img.shields.io/badge/Maintained%3F-yes-green.svg
    :target: https://GitHub.com/lagcleaner/tfprotocol_client_py/graphs/commit-activity

.. image:: https://img.shields.io/github/issues/lagcleaner/tfprotocol_client_py.svg
    :target: https://GitHub.com/lagcleaner/tfprotocol_client_py/issues/

.. image:: https://img.shields.io/github/issues-closed/lagcleaner/tfprotocol_client_py.svg
    :target: https://GitHub.com/lagcleaner/tfprotocol_client_py/issues?q=is%3Aissue+is%3Aclosed

----------------
Introduction 
----------------

The especifications for the *Transference Protocol* is available in this `repository
<https://github.com/GoDjango-Development/TFProtocol/blob/main/doc/>`_.


---------------------------
Installation :floppy_disk:
---------------------------

.. image:: https://img.shields.io/pypi/v/tfprotocol-client.svg
    :target: https://pypi.org/project/tfprotocol-client/

The package is available at `pypi <https://pypi.org/project/tfprotocol-client/>`_, to be installed from **pip** with the
next command:

.. code-block:: bash

    pip install tfprotocol_client

-------------------------
A Simple Example :memo:
-------------------------

To use the *Transference Protocol* through this library, you must create an instance of
*TfProtocol* with the specified parameters and have an online server to connect to.

.. code-block:: python

    from tfprotocol_client.misc.constants import RESPONSE_LOGGER
    from tfprotocol_client.tfprotocol import TfProtocol

    ADDRESS = 'tfproto.expresscuba.com'
    PORT = 10345
    clienthash = '<clienthash>'
    publickey = '<publickey>'

    proto = TfProtocol('0.0', publickey, clienthash, ADDRESS, PORT)
    proto.connect()
    proto.echo_command('Hello World', response_handler=RESPONSE_LOGGER)
    proto.disconnect()


---------------------------
For Contributors :wrench:
---------------------------

.. image:: https://img.shields.io/github/contributors/lagcleaner/tfprotocol_client_py.svg
    :target: https://GitHub.com/lagcleaner/tfprotocol_client_py/graphs/contributors/

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Development Environment Installation :computer:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To set up the development environment, all you need as a prerequisite is to have Python 2.7
or 3.5+ and `poetry <https://python-poetry.org/>`_ installed. If you need to install poetry
follow `these steps <https://python-poetry.org/docs/#installation>`_ and come back. 

With this in mind, to install the necessary dependencies and create a python environment for
this project, proceed to run the following command in the root directory of the project.

.. code-block:: bash

    poetry install


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Project Structure :open_file_folder:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This library is made up of 5 folders and the particular implementations of the ``TfProtocolSuper``
class, the folders are structured as follows:

- **connection**: where all socket and low-level communication is located.
- **models** where the complex objects used all over the package are defined.
- **security** where is implemented the methods and classes to encrypt and decrypt the messages for communication and also the utils for do the hashing stuff where is needed.
- **misc** to hold all utils and not related to any other folder concept.
- **extensions** where are all the extensions for the *Transfer Protocol Client* .

Here the visual schema for all the classes and his relations with others:

.. image:: ./doc/statics/classes.png
    :alt: class relations
    :align: center

^^^^^^^^^^^^^^^^^^^^
Publishing :rocket:
^^^^^^^^^^^^^^^^^^^^

To publish the package you need to run the following command in the root directory of the package:

.. code-block:: bash

    poetry publish

.. image:: https://img.shields.io/badge/Ask%20me-anything-1abc9c.svg
    :target: mailto://lagcleaner@gmail.com
