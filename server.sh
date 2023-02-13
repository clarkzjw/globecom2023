export SSLKEYLOGFILE="./tmp/key"
./tserver -M 2 -q qlog -c ./ca/ca-cert.pem -k ./ca/server-key.pem -p 4443 -w /home/clarkzjw/Documents/dataset/mpd
#./tserver -M 2 -q qlog -c ./ca/ca-cert.pem -k ./ca/server-key.pem -p 4443 -w /home/clarkzjw/Documents/sync/UVic/PanLab/Code/picoquic/dataset/BigBuckBunny/mpd
