export SSLKEYLOGFILE="./tmp/key"
./tserver -M 2 -q qlog -c ./ca-cert.pem -k ./ca-key.pem -p 443 -w ~/clarkzjw-globecom23/mpd
