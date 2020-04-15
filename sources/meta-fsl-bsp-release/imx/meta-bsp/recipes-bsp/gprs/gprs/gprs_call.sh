sleep 5
echo "nameserver 114.114.114.114" >> /etc/resolv.conf
echo "nameserver 8.8.8.8" >> /etc/resolv.conf
wvdial 3g &
sleep 2
route_rule
