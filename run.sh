#!/bin/sh

./rcp_rc -s -f $1 -d $2 &

sleep 1

ssh teaker-6 'cd /home/mateng/rdma/rcp; a=`./rcp_rc -c -i 10.0.0.5`; echo -e "\e[1;31m ${a} \e[0m" '
