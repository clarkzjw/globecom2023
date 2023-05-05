#!/usr/bin/env bash

set -x

epoch=50
nb_segment=200

exp_id=roundrobin

screen -dmS roundrobin bash -c "epoch=$epoch; exp_id=$exp_id; bash << 'EOF'
for i in {1..$epoch}
do
    python3 mininet/topo.py --exp_id \"$exp_id\"-RR --scheduler roundrobin --nb_segment \"$nb_segment\"
done
EOF
"

epoch=50
nb_segment=200

exp_id=minrtt

screen -dmS minrtt bash -c "epoch=$epoch; exp_id=$exp_id; bash << 'EOF'
for i in {1..$epoch}
do
    python3 mininet/topo.py --exp_id \"$exp_id\"-minRTT --scheduler minrtt --nb_segment \"$nb_segment\"
done
EOF
"

epoch=50
nb_segment=200

exp_id=lints

screen -dmS lints bash -c "epoch=$epoch; exp_id=$exp_id; bash << 'EOF'

declare -a arr=(\"0.2\" \"0.4\" \"0.6\" \"0.8\" \"1.0\")

for alpha in \"\${arr[@]}\"
do
    for i in {1..$epoch}
    do
        python3 mininet/topo.py --scheduler contextual_bandit --exp_id \"$exp_id\"-LinTS-alpha-\"\$alpha\" --lints_alpha \"\$alpha\" --algorithm LinTS --nb_segment \"$nb_segment\"
    done
done
EOF
"

# linucb
epoch=50
nb_segment=200

exp_id=linucb

screen -dmS linucb bash -c "epoch=$epoch; exp_id=$exp_id; bash << 'EOF'

declare -a arr=(\"0.2\" \"0.4\" \"0.6\" \"0.8\" \"1.0\")

for alpha in \"\${arr[@]}\"
do
    for i in {1..$epoch}
    do
        python3 mininet/topo.py --scheduler contextual_bandit --exp_id \"$exp_id\"-LinUCB-alpha-\"\$alpha\" --linucb_alpha \"\$alpha\" --algorithm LinUCB --nb_segment \"$nb_segment\"
    done
done
EOF
"
