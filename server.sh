export SSLKEYLOGFILE="./tmp/key"
#./tserver -M 2 -q qlog -c ./ca/ca-cert.pem -k ./ca/server-key.pem -p 4433 -w /home/clarkzjw/Documents/sync/UVic/PanLab/Code/picoquic/videos/svc
./tserver -M 2 -q qlog -c ./ca/ca-cert.pem -k ./ca/server-key.pem -p 4433 -w ./videos/BigBuckBunny/mpd