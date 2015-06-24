# LD_LIBRARY_PATH=~/checkouts/libboost-1.55.install/lib PATH=$PATH:~/checkouts/sslapi/client_server-build make -f ~/checkouts/sslapi/admin/Makefile test

caCN?=myca
serverCN?=myserver
csrCN?=mycsr
cakey?=CAkeyPassPhrase
port?=8088
path?=SSLAPI

## Setting up a CA
certs private:
	mkdir certs private

# Create certificate db
serial:
	echo 100001 >serial

certindex.txt certindex.txt.attr:
	touch certindex.txt certindex.txt.attr

# Create a configuration file in the main directory
openssl.conf:
	curl -o openssl.conf https://raw.githubusercontent.com/ericprud/sslapi/master/admin/sample-openssl.conf

private/cakey.pem ca.crt: openssl.conf
	echo "\n\n\n$(caCN)" | openssl req -new -x509 -extensions v3_ca -keyout private/cakey.pem -out ca.crt -days 3650 -config ./openssl.conf -passout pass:$(cakey)

## Create a server certificate
# Create CSR
server.csr private/server.pem: openssl.conf private
	echo "\n\n\n$(serverCN)" | openssl req -new -nodes -out server.csr -keyout private/server.pem -days 3650 -config ./openssl.conf

# Sign the CSR
server.crt: openssl.conf server.csr private/cakey.pem certindex.txt serial
	yes | openssl ca -passin pass:$(cakey) -out server.crt -days 3650 -config ./openssl.conf -infiles server.csr

##  Create a client certificate
# Create CSR
client.csr private/client.pem: openssl.conf
	echo "\n\n\n$(csrCN)" | openssl req -new -nodes -out client.csr -keyout private/client.pem -days 3650 -config ./openssl.conf

# Sign the CSR
client.crt: openssl.conf client.csr
	yes | openssl ca -passin pass:$(cakey) -out client.crt -days 3650 -config ./openssl.conf -infiles client.csr

# Create key (called dh512.pem for no reason i can think of)
dh512.pem:
	openssl dhparam -out dh512.pem -2 1024

## build run environment
# server environment
run run/server run/client:
	mkdir -p run/server run/client

run/server/server.pem: server.crt private/server.pem | run/server
	cat server.crt private/server.pem > run/server/server.pem

run/server/dh512.pem: dh512.pem | run/server
	ln -s ../../dh512.pem run/server/dh512.pem

run/server/ca.crt: ca.crt | run/server
	ln -s ../../ca.crt run/server/ca.crt

# client environment
run/client/client.pem: private/client.pem | run/client
	ln -s ../../private/client.pem run/client/client.pem

run/client/client.crt: client.crt | run/client
	ln -s ../../client.crt run/client/client.crt

## run an SSL server
runSSLserver run/server/pid:
	(cd run/server && openssl s_server -accept $(port) -CAfile ca.crt -dkey dh512.pem -cert server.pem -verify ca.crt -www)&
	sleep 1 && touch run/server/pid

## run the compiled server
runServer run/server/pid: run/server/server.pem run/server/dh512.pem run/server/ca.crt
	(cd run/server && server --serve http://localhost:$(port)/$(path))&
	sleep 1 && touch run/server/pid

## run curl client
runCurl: run/client/client.pem run/client/client.crt run/server/pid
	(cd run/client && curl -k --cert client.crt --key client.pem https://localhost:$(port)/$(path))

runClient-noworky: run/client/client.crt run/client/client.key $(cakey) run/server/ca.crt run/server/dh512.pem
	client.exe localhost $(port) client.crt client.key $(cakey) ca.crt dh512.pem


## tests
test0: runSSLserver runCurl
	killall openssl
	rm run/server/pid

test1: runServer runCurl
	killall server
	rm run/server/pid

