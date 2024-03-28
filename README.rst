================================================
TFProtocol :dog: 
================================================

**You can donate in BTC to help us maintain and drive these products for the benefit of all of us.**

Bitcoin Wallet: 129ziaFyteW1rGzTcjcPD644fn4RC9ayh9

.. image:: https://jenkins.godjango.dev/buildStatus/icon?job=Tfprotocol
    :target: https://jenkins.godjango.dev/buildStatus/icon?job=Tfprotocol

.. image:: https://img.shields.io/badge/Maintained%3F-yes-green.svg
    :target: https://github.com/GoDjango-Development/TFProtocol/graphs/commit-activity

.. image:: https://img.shields.io/github/license/GoDjango-Development/TFProtocol.svg
    :target: https://github.com/GoDjango-Development/TFProtocol/blob/master/LICENSE

.. image:: https://img.shields.io/github/release/GoDjango-Development/TFProtocol.svg
    :target: https://github.com/GoDjango-Development/TFProtocol/releases/

.. image:: https://img.shields.io/github/issues/GoDjango-Development/TFProtocol.svg
    :target: https://img.shields.io/github/release/GoDjango-Development/TFProtocol/issues/

.. image:: https://badgen.net/badge/icon/docker?icon=docker&label
    :target: https://https://docker.com/

.. image:: https://badgen.net/badge/Open%20Source%20%3F/Yes%21/blue?icon=github
    :target: https://github.com/GoDjango-Development/TFProtocol/

.. toctree::
    CHANGELOG.md
    LICENSE.rst
    clients/java/README.rst Java Implementation
    clients/java/docs/ace.md
    clients/java/docs/index.md
    clients/python/README.rst


------------------------------------------
What is TFProtocol:
------------------------------------------

    Tfprotocol is a communcation protocol to execute data in remote servers like ssh, its main difference are, tfprotocol is statefull, lightweight, multithread and multiprocess 
    have support for android and desktop as well.

-----------------------------------------
How to use
-----------------------------------------

`Docker`: When using docker you must first notice that we use some environment to customize your docker running application, here is a list of all docker enviornment variables and its respectives description: 

    - PORT: The port in which tfprotocol will be running (default: 10345)
    - HASH: A hash that works for identificate the server instance (default: testhash)
    - PROTO: The version of tfprotocol to be used (default: 0.0)
    - DBDIR: The path to the tfprotocol db folder, which means the root of the tfprotocol program, where all files are stored and no command can go out of that folder (default: /var/tfdb)
    - XSNTMEX: The path to the XSNTMEX subsystem host folder (default: /var/tfdb/xsntmex/)
    - USERDB: The path to the user db, NOTE: More information will be given later. (default: /var/tfdb/lsr/)
    - XSACE: The path to the XSACE substystem (default: /var/tfdb/xsace/)
    - DEFUSER: The default user toUnexpected be used by the protocol (default: root)
    - TLB: The path to the TLB substystem (default: /var/tfdb/tlb/)
    - SECUREFS: If the protocol should use a secure filesystem or not (default: true) 
    - RPCPROXY: The path to the rpcproxy substystem (default: /var/tfdb/rpcproxy)
    
    Command to run:
    
    .. code-block:: bash

            docker run -e PORT=10345 -e HASH=testhash ... -p 10345:10345 -d etherbeing/tfprotocol 
    
    .. code-block:: yaml

            Docker Compose:

                version: "3.6"

                services:
                    tfprotocol:
                        image: etherbeing/tfprotocol
                        restart: unless-stopped
                        environment: 
                            - PORT=10345
                            - DEBUG=false # use this if you want to say how each command interact with the server, do not use this in production
                        ports: 
                            - 10345:10345
                        volumes: 
                            - tfdb:/var/tfdb
                        networks:
                            - tfprotocol
                        secrets:
                            - tfprotocol_keys

                volumes:
                    tfdb:
                        driver: local
                networks:
                    tfprotocol:
                        driver: bridge
                secrets:
                    tfprotocol_keys:
                        file: /mycustom/path/to/mycustom/keys 

--------------------------------------
Executing modes:
--------------------------------------

    - Debug mode
    - Debug mode (udp only)
    - Release mode (interactive)
    - Release mode (daemon: canonical)

#################
Debug mode
#################

debug/tfd debug/file.conf

this runs tfprotocol binary in interactive mode (you may press ctrl+c to stop
it)

#################
Debug mode udp
#################

debug/tfd debug/file.conf udp_debug

this runs tfprotocol binary in interactive mode but it only runs the udp server.

SIGNALS

 -SIGINT
    Will gracefully close TFProtocol daemon disconnectinig it from the network.

########################################
Release mode (interactive)
########################################

/absolute/path/release/tfd /absolute/path/release/file.conf

this runs tfprotocol binary in interactive mode (you may press ctrl+c to stop
it)

SIGNALS

 -SIGINT
    Will gracefully close TFProtocol daemon disconnectinig it from the network.

 -SIGUSR1
    Will reload TFProtocol daemon with the same configuration that start it.

############################################
Release mode (daemon: canonical)
############################################

/absolute/path/release/tfd /absolute/path/release/file.conf x

this runs tfprotocol binary as intended for production environment. It does a
double fork and stay running in background.

SIGNALS

 -SIGINT
    Will gracefully close TFProtocol daemon disconnectinig it from the network.

 -SIGUSR1
    Will reload TFProtocol daemon with the same configuration that start it.
    
In release mode, interactive or canonical, the exec and config path both must be
absolute path to allow hot-reload on SIGUSR1 signal.
    
---------------------------------------
TFProtocol Configuration file.
---------------------------------------

Any line that starts with the # character will be taken as comment and will be
omitted.

`proto 0.0`

This parameter is required and sets the version of the deployed protocol. It
could be any string.

`hash testhash`

This parameter is required and sets a token that can be any string, this 
provides another access control level to clients. 

`dbdir /var/tfdb/`

This parameter is required and sets the directory in which TFprotocol and most 
of its extended subsystems will be locked.

`faipath /var/run/tfproto_fai`

This parameter is required and sets the directory in which TFprotocol implemetns
the Fast Access Interface. In this directory FAI tokens will be stored.

`faitok_mq 500`

This parameter is optional and sets the maximun quanta for a token to remain
valid. If not set, a default value will be used instead.

`port 10345`

This parameter is required and sets the port in which TFProtocol will listen for
upd and tcp incoming connections.

`privkey {the rsa private key}`

This parameter is required and encloses in braces the rsa server private key.

`xsntmex /var/tfdb/xsntmex`

This parameter is optional and establish the file of access tokens for the NTMEX
extended subsystem.

`userdb /var/tfdb/lsr`

This parameter is optional and sets user database file in which TFProtocol will
look for LOGIN command autentication.

    - xsace /var/tfdb/xsace

This parameter is optional and sets the ACE extended subsystem access tokens 
file.

`defusr user`

This parameter is optional and sets the default system user -user id- in which 
TFProtocol will run while communicates with clients. If this option is not set 
then the client have to autenticate through the LOGIN command as soon as is 
connected to TFProtocol.


`tlb /var/tfdb/tlb`

This parameter is optional and establish the file of for the Transfer Load 
Balance servers. This file will be used both for the TCP and UDP TLB.

`pubkey {rsa server public key}`
    
`pubkey`

This parameter is optional and sets the rsa server public key. It will be used 
when the Transparent Proxy is in place which requires that the destination 
server has the same rsa private/public key pairs.

`trp_dns; trp_ipv4; trp_ipv6``

This parameter is optional and sets the Transparanet Proxy destination 
ip or dns.

`injail`

This parameter is optional and tells the tfprotocol server to compell users to
call INJAIL command. 
There must be a file in the injailed directory called jail.key

jail.key file syntax:

- jail_token

How the optional parameters works

`xsntmex /var/tfdb/xsntmex`
    
xsntmex file syntax:
(only line per access token.)

- token1
- token2

in the same directory of xsntmex file for each line of token should be a file
ending by .acl like below:

- xsntmex.token1.acl
- xsntmex.token2.acl

each one of the acl file must have the allowed shared object path as follow:

- /usr/lib/libc.so
- /usr/lib/libx.so

if the xsntmex.token1.acl file does not exist, then any shared object for that 
security token can be loaded under NTMEX substystem.

`userdb /var/tfdb/lsr`

userdb file syntax:
(one line per user/password)

- username password

the username must by a real unix system user while password is not.

`xsace /var/tfdb/xsace`

xsace file syntax:
(one line per access token.)

- token1
- token2

in the same directory of xsace file for each line of token can be a file
ending by .acl like below:

- xsace.token1.acl
- xsace.token2.acl

each one of the acl file must have the allowed binary to be executed:

example 1:

- /bin/ls /bin/ls /

The /bin/ls binary can only be executed with two parameters, the first one:
/bin/ls, the second one: /

example 2:

- \*/bin/ls

The /bin/ls binary can only be executed with any parameters.

if the xsace.token1.acl file does not exist, then any program for that security
token can be executed under ACE substystem.

`tlb /var/tfdb/tlb`

Syntax for the TLB file

- ipv4: 127.0.0.1
- ipv6: ::1

`trp`
    
examples:

- trp_ipv4 127.0.0.1
- trp_ipv6 ::1
- trp_dns domain.com

`ijnail`
    
The directory in which the daemon will be in-jailed must contain a file called
jail.key and inside of it must exist the token passed as parameter for injail
command.

`locksys`

If present, LOCKSYS command should be called before any other, except: END; LOGIN; 
KEEPALIVE; PROCKEY; INJAIL, to specify a directory in which TFProtocol must lock
the client. This is called Folder Locking System.

`rpcproxy /var/tfdb/rpcproxy`
    
This file will contain the hash/binary pairs that XS_RPCPROXY subsystem is able
to execute.

Syntax for the rpcproxy file

SHA256 /usr/bin/python /home/user/mypython.py
SHA256 /usr/bin/binary

`nprocmax -1`

This parameter is optional and if is present the TFProtocol daemon will try to
set the MAX USER PROCESSES (how many time may do FORK()) to the specified
number. Negative value means infinite. Zero (0) means the default system
configuration for the effective user id of the daemon. A number between 1 and
MINPROCN macro will become in MINPROCN value. A number equal or less than
MINPROCN will become that number. Be aware that the OS can enforce some settings
that makes this number useless. This option in Solaris will no have effect.
In Linux if this number is above kernel.threads-max and/or kernel.pid_max the
number become kernel.threads-max implicitly.

securefs

This parameter is optional and if is present the TFProtocol daemon will enforce
TFProtocol Secure Filesystem.

runbash

This parameter is optional and if is present the TFProtocol daemon will allow
to run the RUNBASH command which is hihgly sensitive and must be only activated
if the sysadmin has very clear what is intended for. Please check the 
tfproto.pdf document to fully understand what are the implications of this.

flycontext

This parameter is optional and if is present the TFProtocol daemon will create
the specified directory recursively and injail the daemon in that directory.

------------------
    Mantainers
------------------

.. image:: https://img.shields.io/badge/maintainer-n4b3ts3-blue
    :target: mailto://esteban@godjango.dev

.. image:: https://img.shields.io/badge/maintainer-lmdelbahia-blue
    :target: mailto://luismiguel@godjango.dev

----

.. image:: https://img.shields.io/badge/Ask%20me-anything-1abc9c.svg
    :target: https://GitHub.com/GoDjango-Development/issues/

