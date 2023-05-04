dir="/app/player"
export SSLKEYLOGFILE="$dir/tmp/key"
$dir/tserver -M 2 -q qlog -c $dir/ca/ca-cert.pem -k $dir/ca/server-key.pem -p 443 -w /app/mpd
