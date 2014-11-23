#!/bin/bash

# if following yes/no variable is equal to "y"
# then it means yes
# else it means no

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

# Auto Install config End