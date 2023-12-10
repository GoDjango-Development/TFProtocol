# Before using docker build . -t tfprotocol please use make prepare and make release in the project root 
FROM debian:latest
LABEL org.opencontainers.image.authors="esteban@godjango.dev"
LABEL version="2.4.3"
LABEL description="TFProtocol, ready to use server..."
ARG PORT=10345
ENV HASH=testhash
ENV PORT=$PORT
ENV PROTO=0.0
ENV DBDIR=/var/tfdb/
ENV XSNTMEX=/var/tfdb/xsntmex/
ENV USERDB=/var/tfdb/lsr/
ENV XSACE=/var/tfdb/xsace/
ENV DEFUSER=root
ENV TLB=/var/tfdb/tlb/
ENV SECUREFS=true
ENV RPCPROXY=/var/tfdb/rpcproxy
ENV DEBUG=false
# Copy the entrypoint file to /startup.py
COPY container/startup.py /startup.py
# Copy the daemon binary to the container
COPY release/tfd /usr/local/bin/tfd
# Copy the daemon debug binary to the container
COPY debug/tfd /usr/local/bin/tfd_debug
# Creates the configuration folder
RUN mkdir /root/.tfprotocol/
# Copy the configuration to the container
COPY conf/example.conf /root/.tfprotocol/release.conf
# Install openssl and generate the RSA keypair
RUN apt-get update && apt-get install openssl python3 libmariadb-dev libpq-dev libsqlite3-dev -y &&  openssl genrsa -out /root/.tfprotocol/private.pem 2048 && openssl rsa -in /root/.tfprotocol/private.pem -pubout -out /root/.tfprotocol/public.pem && python3 /startup.py && chmod +x /usr/local/bin/tfd && mkdir /var/tfdb/ # run the startup that creates the actual configuration file
# RUN python3 /startup.py
EXPOSE ${PORT}
ENTRYPOINT  ["/usr/bin/env", "-S python3 /startup.py"]
#CMD ["/usr/local/bin/tfd", "/root/.tfprotocol/release.conf"]
