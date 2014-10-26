#!/bin/bash

# config start
IS_AUTO_INSTALL=y

# following setting will only work
# only when IS_AUTO_INSTALL=y

# Outgoing network is needed for download CodFS
SET_PROXY=y

# proxy setting
SET_HTTP_PROXY="http://proxy.cse.cuhk.edu.hk:8000/" 
SET_HTTPS_PROXY="http://proxy.cse.cuhk.edu.hk:8000/" 
SET_FTP_PROXY="http://proxy.cse.cuhk.edu.hk:8000/"

# save proxy setting permanently
# to apply setting, please reboot the computer
PERNAMENT_PROXY=y

# if following yes/no variable is equal to "y"
# then it means yes
# else it means no


# Auto Install config End

#backup
## function you may useful
#function findNICName {
#	ip a | grep inet\  | grep  `echo $1 | sed "s/\([0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.\)[0-9]\{1,3\}/\1/"` | sed 's/^.*\ \([0-9a-zA-Z]\+\)$/\1/'
#}
#function getNum {
#        local result
#        while true; do
#                read -p "$1" result
#                case `echo $result | sed 's/[0-9]//g'` in
#                        '' ) echo $result; break;;
#                esac
#        done
#}

