export SSLKEYLOGFILE="./tmp/key"
./tserver -M 2 -q qlog -c ./ca-cert.pem -k ./ca-key.pem -p 4443 -w ../mpd
