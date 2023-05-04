set -x

epoch=20
nb_segment=200

exp_id=20230428-bw_h-100-bw_l-30-dynamic-roundrobin

screen -dmS roundrobin bash -c "epoch=$epoch; exp_id=$exp_id; bash << 'EOF'
for i in {1..$epoch}
do
    echo \"\$i\"
    python3 mininet/topo.py --exp_id \"$exp_id\"-RR --scheduler roundrobin --nb_segment \"$nb_segment\"
done
EOF
"

epoch=20
nb_segment=200

exp_id=20230428-bw_h-100-bw_l-30-dynamic-minrtt

screen -dmS minrtt bash -c "epoch=$epoch; exp_id=$exp_id; bash << 'EOF'
for i in {1..$epoch}
do
    echo \"\$i\"
    python3 mininet/topo.py --exp_id \"$exp_id\"-minRTT --scheduler minrtt --nb_segment \"$nb_segment\"
done
EOF
"

epoch=20
nb_segment=200

exp_id=20230428-bw_h-100-bw_l-30-dynamic-lints

screen -dmS lints bash -c "epoch=$epoch; exp_id=$exp_id; bash << 'EOF'

declare -a arr=(\"0.2\" \"0.4\" \"0.6\" \"0.8\" \"1.0\")

## now loop through the above array
for alpha in \"\${arr[@]}\"
do
    for i in {1..$epoch}
    do
        echo \"\$i, \$alpha\" >> log.txt
        python3 mininet/topo.py --scheduler contextual_bandit --exp_id \"$exp_id\"-LinTS-alpha-\"\$alpha\" --lints_alpha \"\$alpha\" --algorithm LinTS --nb_segment \"$nb_segment\"
    done
done
EOF
"


# linucb
epoch=20
nb_segment=200

exp_id=20230428-bw_h-100-bw_l-30-dynamic-linucb

screen -dmS linucb bash -c "epoch=$epoch; exp_id=$exp_id; bash << 'EOF'

declare -a arr=(\"0.2\" \"0.4\" \"0.6\" \"0.8\" \"1.0\")

## now loop through the above array
for alpha in \"\${arr[@]}\"
do
    for i in {1..$epoch}
    do
        echo \"\$i, \$alpha\" >> log.txt
        python3 mininet/topo.py --scheduler contextual_bandit --exp_id \"$exp_id\"-LinUCB-alpha-\"\$alpha\" --linucb_alpha \"\$alpha\" --algorithm LinUCB --nb_segment \"$nb_segment\"
    done
done
EOF
"
