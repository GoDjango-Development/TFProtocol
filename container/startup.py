#!/usr/bin/env python3 


import os
conf_path = None

def generate_keypair():
    """
        -------------------------------
        Key Pair generator
        -------------------------------
        The key pair generator understand a key pair file syntax as follows:
        .. code-block: none
                # key_pair
                FIRST_KEY 

                SECOND_KEY
        Notice in the previous example that the keys are separated by two \n visually only one cause the other one is at the end of the FIRST_KEY
    """
    conf_folder = os.path.join(os.getenv("HOME"), ".tfprotocol")
    if os.path.exists("/run/secrets/"):
        files = os.listdir("/run/secrets")
        for file in files:
            with open(f"/run/secrets/{file}", "r") as opened_file:
                first_line = opened_file.readline()
                if first_line.startswith("#") and "key_pair" in first_line:
                    data = opened_file.read().split("\n\n")
                    data = list(filter(lambda elem: len(elem.strip()) > 0, data))
                    key_pair = {
                        "public_key": None,
                        "private_key": None
                    }
                    for element in data:
                        if "PRIVATE KEY" in element:
                            key_pair["private_key"] = element
                        elif "PUBLIC KEY" in element:
                            key_pair["public_key"] = element
                        if all(key_pair.values()):
                            with open(os.path.join(conf_folder, "private.pem"), "w") as private_key:
                                private_key.write(key_pair["private_key"])
                            with open(os.path.join(conf_folder, "public.pem"), "w") as public_key:
                                public_key.write(key_pair["public_key"])
                            return
                    raise EOFError("We reach the end of file and there wasnt any identifiable key, please ensure your keys are wrapped with PUBLIC KEY and PRIVATE KEY respectively")
    else:
        os.system(f"openssl genrsa -out {os.path.join(conf_folder, 'private.pem')} 2048")
        os.system(f"openssl rsa -in {os.path.join(conf_folder, 'private.pem')} -pubout -out {os.path.join(conf_folder, 'public.pem')}")

def generate():
    global conf_path
    conf_folder = os.path.join(os.getenv("HOME"), ".tfprotocol")
    release_file = os.path.join(conf_folder, "release.conf")
    privkey_file = open(os.path.join(conf_folder, "private.pem"), "r")
    pubkey_file = open(os.path.join(conf_folder, "public.pem"), "r")
    disable_all = os.getenv("DISABLE_SUBMODULES")
    try:
        with open(release_file, "w") as file:
            conf_path = file.name
            file.write(f"""
proto {os.getenv("PROTO")}
hash {os.getenv("HASH")}
dbdir {os.getenv("DBDIR")}
port {os.getenv("PORT")}
privkey \{{privkey_file.read()}\}
{'#' if disable_all else ''}xsntmex {os.getenv("XSNTMEX")}
{'#' if disable_all else ''}userdb {os.getenv("USERDB")}
{'#' if disable_all else ''}xsace {os.getenv("XSACE")}
defusr {os.getenv("DEFUSER")}
{'#' if disable_all else ''}tlb {os.getenv("TLB")}
pubkey \{pubkey_file.read()\}
{os.getenv("SECUREFS") if "" else "#"} securefs
{os.getenv("LOCKSYS") if "" else "#"} locksys
{'#' if disable_all else ''}rpcproxy {os.getenv("RPCPROXY")}
""")
    except FileExistsError:
        pass
    privkey_file.close()
    pubkey_file.close()

if __name__ == "__main__":
    generate_keypair()
    generate()
    if os.getenv("DEBUG") == "true":
        os.system("/usr/local/bin/tfd_debug %s"%conf_path)
    else:
        os.system("/usr/local/bin/tfd %s"%conf_path)
