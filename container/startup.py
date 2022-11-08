#!/usr/bin/env python3 


import os
conf_path = None

def generate():
    global conf_path
    conf_folder = os.path.join(os.getenv("HOME"), ".tfprotocol")
    release_file = os.path.join(conf_folder, "release.conf")
    privkey_file = open(os.path.join(conf_folder, "private.pem"), "r")
    pubkey_file = open(os.path.join(conf_folder, "public.pem"), "r")
    try:
        with open(release_file, "x") as file:
            conf_path = file.name
            file.write("""
proto %s
hash %s
dbdir %s
port %s
privkey %s
xsntmex %s
userdb %s
xsace %s
defusr %s
tlb %s
pubkey %s
%s securefs
%s locksys
rpcproxy %s
"""%(
        os.getenv("PROTO"),
        os.getenv("HASH"),
        os.getenv("DBDIR"),
        os.getenv("PORT"),
        privkey_file.read(),
        os.getenv("XSNTMEX"),
        os.getenv("USERDB"),
        os.getenv("XSACE"),
        os.getenv("DEFUSR"),
        os.getenv("TLB"),
        pubkey_file.read(),
        os.getenv("SECUREFS") if "" else "#",
        os.getenv("LOCKSYS") if "" else "#",
        os.getenv("RPCPROXY")
    ))
    except FileExistsError:
        pass
    privkey_file.close()
    pubkey_file.close()

if __name__ == "__main__":
    generate()
    #os.system("/usr/local/bin/tfd %s"%conf_path)
