sudo tc qdisc del dev enp0s3 root 2>/dev/null
sudo tc qdisc add dev enp0s3 root handle 1:0 netem delay 20ms loss 50%
sudo tc qdisc add dev enp0s3 parent 1:1 handle 10: tbf rate 20Mbit burst 10mb latency 1ms
