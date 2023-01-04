================================================
TFProtocol Server :dog: 
================================================

.. image:: https://github.com/GoDjango-Development/TFProtocol/actions/workflows/test_and_build.yml/badge.svg
    :target: https://github.com/GoDjango-Development/TFProtocol/actions/workflows/test_and_build.yml

.. image:: https://svgshare.com/i/Zhy.svg
    :target: https://svgshare.com/i/Zhy.sv

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


Executing modes:

    - Debug mode
    - Debug mode (udp only)
    - Release mode (interactive)
    - Release mode (daemon: canonical)

- Debug mode

debug/tfd debug/file.conf

this runs tfprotocol binary in interactive mode (you may press ctrl+c to stop
it)

- Debug mode udp

debug/tfd debug/file.conf udp_debug

this runs tfprotocol binary in interactive mode but it only runs the udp server.

- Release mode (interactive)

release/tfd release/file.conf

this runs tfprotocol binary in interactive mode (you may press ctrl+c to stop
it)

- Release mode (daemon: canonical)

release/tfd release/file.conf x

this runs tfprotocol binary as intended for production environment. It does a
double fork and stay running in background.

- TFProtocol Configuration file.

Any line that starts with the # character will be taken as comment and will be
omitted.

    proto 0.0

This parameter is required and sets the version of the deployed protocol. It
could be any string.

    hash testhash

This parameter is required and sets a token that can be any string, this 
provides another access control level to clients. 

    dbdir /var/tfdb/

This parameter is required and sets the directory in which TFprotocol and most 
of its extended subsystems will be locked.

    port 10345

This parameter is required and sets the port in which TFProtocol will listen for
upd and tcp incoming connections.

    privkey {the rsa private key}

This parameter is required and encloses in braces the rsa server private key.

    - xsntmex /var/tfdb/xsntmex

This parameter is optional and establish the file of access tokens for the NTMEX
extended subsystem.

    - userdb /var/tfdb/lsr

This parameter is optional and sets user database file in which TFProtocol will
look for LOGIN command autentication.

    - xsace /var/tfdb/xsace

This parameter is optional and sets the ACE extended subsystem access tokens 
file.

    - defusr user

This parameter is optional and sets the default system user -user id- in which 
TFProtocol will run while communicates with clients. If this option is not set 
then the client have to autenticate through the LOGIN command as soon as is 
connected to TFProtocol.


    - tlb /var/tfdb/tlb

This parameter is optional and establish the file of for the Transfer Load 
Balance servers. This file will be used both for the TCP and UDP TLB.

    - pubkey {rsa server public key}
    
pubkey

This parameter is optional and sets the rsa server public key. It will be used 
when the Transparent Proxy is in place which requires that the destination 
server has the same rsa private/public key pairs.

    - trp_dns; trp_ipv4; trp_ipv6

This parameter is optional and sets the Transparanet Proxy destination 
ip or dns.

    - injail

This parameter is optional and tells the tfprotocol server to compell users to
call INJAIL command. 
There must be a file in the injailed directory called jail.key

jail.key file syntax:

jail_token


    ::::How the optional parameters works::::

    - xsntmex /var/tfdb/xsntmex
    
xsntmex file syntax:

onle line per access token.

token1
token2

in the same directory of xsntmex file for each line of token should be a file
ending by .acl like below:

xsntmex.token1.acl
xsntmex.token2.acl

each one of the acl file must have the allowed shared object path as follow:

/usr/lib/libc.so
/usr/lib/libx.so

if the xsntmex.token1.acl file does not exist, then any shared object for that 
security token can be loaded under NTMEX substystem.

     - userdb /var/tfdb/lsr

userdb file syntax:

one line per user/password:
username password

the username must by a real unix system user while password is not.

    - xsace /var/tfdb/xsace

xsace file syntax:

one line per access token.

token1
token2

in the same directory of xsace file for each line of token can be a file
ending by .acl like below:

xsace.token1.acl
xsace.token2.acl

each one of the acl file must have the allowed binary to be executed:

example 1:

/bin/ls /bin/ls /

The /bin/ls binary can only be executed with two parameters, the first one:
/bin/ls, the second one: /

example 2:

*/bin/ls

The /bin/ls binary can only be executed with any parameters.

if the xsace.token1.acl file does not exist, then any program for that security
token can be executed under ACE substystem.

    - tlb /var/tfdb/tlb

Syntax for the TLB file

4 127.0.0.1;1111
6 ::1;1111

    - trp
    
examples:

trp_ipv4 127.0.0.1
trp_ipv6 ::1
trp_dns domain.com

    - ijnail
    
The directory in which the daemon will be in-jailed must contain a file called
jail.key and inside of it must exist the token passed as parameter for injail
command.

    - locksys

If present, LOCKSYS command should be called before any other, except: END; LOGIN; 
KEEPALIVE; PROCKEY; INJAIL, to specify a directory in which TFProtocol must lock
the client. This is called Folder Locking System.

    - rpcproxy /var/tfdb/rpcproxy
    
This file will contain the hash/binary pairs that XS_RPCPROXY subsystem is able
to execute.

Syntax for the rpcproxy file

SHA256 /usr/bin/python /home/user/mypython.py
SHA256 /usr/bin/binary

    - nprocmax -1

This parameter is optional and if is present the TFProtocol daemon will try to
set the MAX USER PROCESSES (how many time may do FORK()) to the specified
number. Negative value means infinite. Zero (0) means the default system
configuration for the effective user id of the daemon. A number between 1 and
MINPROCN macro will become in MINPROCN value. A number equal or less than
MINPROCN will become that number. Be aware that the OS can enforce some settings
that makes this number useless. This option in Solaris will no have effect.
In Linux if this number is above kernel.threads-max and/or kernel.pid_max the
number become kernel.threads-max implicitly.


----
    Mantainers
----

.. image:: https://img.shields.io/badge/maintainer-n4b3ts3-blue
    :target: mailto://esteban@godjango.dev

.. image:: https://img.shields.io/badge/maintainer-lmdelbahia-blue
    :target: mailto://luismiguel@godjango.dev

----

.. image:: https://img.shields.io/badge/Ask%20me-anything-1abc9c.svg
    :target: https://GitHub.com/GoDjango-Development/issues/

