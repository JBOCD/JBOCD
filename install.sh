#!/bin/bash
echo Setting Function.
function getYN {
	local result
	while true; do
		read -p "$1 (Y/n) " result
		case $result in
			[Yy]* ) echo y; break;;
			[Nn]* ) echo n; break;;
		esac
	done
}
function getUrl {
	local result
	while true; do
		read -p 'Please enter URL:' result
		case $result in
			[hH][tT][tT][pP]{s,S,}://* ) echo $result; break;;
			'' ) echo ''; break;;
		esac
	done
}
function pause {
	echo -n 'To Continue, Press Enter.'
	read
}

echo 'Loading Config File (config.sh)'
if [ -f config.sh ]; then
	. config.sh
fi

if [ $IS_AUTO_INSTALL != y ]; then
	echo Read me!
	echo
	echo This is an install script for CSS.
	echo This script is being developing and
	echo it may need to suspend this script 
	echo to change config file and continue 
	echo the install.
	echo 
	echo Please make sure your computer have
	echo at least 300MB free ram memory for
	echo installation.
	echo 
	pause
fi

echo Setting Environment variable.
BASE_PATH=`pwd`
SAVE_STEP_PATH=${BASE_PATH}/step_info
TEMP=${BASE_PATH}/temp
INITD_PATH=/etc/init.d/
INITD_NAME=CSS
START_SH=${TEMP}/start
STOP_SH=${TEMP}/stop
TEMP_STOP_SH=${TEMP}/temp_stop
OUTPUT_PATH=/usr/local/bin/CSS
#set proxy setting permanently
if [ "$IS_AUTO_INSTALL" != "y" ]; then
	SET_PROXY=`getYN 'Do You Want To Set Proxy?'` 
fi

if [ "$IS_AUTO_INSTALL" != "y" ] && [ "$SET_PROXY" = "y" ]; then
	if [ "`getYN "HTTP Proxy will set to $SET_HTTP_PROXY, Do You want to change?"`" = "y" ]; then
		SET_HTTP_PROXY=`getUrl`
	fi
	if [ "`getYN "HTTPS Proxy will set to $SET_HTTP_PROXY, Do You want to change?"`" = "y" ]; then
		SET_HTTPS_PROXY=`getUrl`
	fi
	if [ "`getYN "FTP Proxy will set to $SET_HTTP_PROXY, Do You want to change?"`" = "y" ]; then
		SET_FTP_PROXY=`getUrl`
	fi
	PERMANENT_PROXY=`getYN "Do You Want To Set Proxy Permanent?"`
fi


echo Starting installing

[ ! -d "$SAVE_STEP_PATH" ] && mkdir "$SAVE_STEP_PATH"
[ -d "$TEMP" ] && (sudo umount $TEMP; sudo rm -r $TEMP); mkdir $TEMP

sudo mount -t tmpfs -o size=300m tmpfs $TEMP
if [ "$SET_PROXY" = "y" ]; then
	echo Changing Proxy Setting
	cd "$SAVE_STEP_PATH"
	tmp=0
	if [ "`cat /etc/environment | grep http_proxy`" != "http_proxy=\"$SET_HTTP_PROXY\"" ];
	then
		if [ "$PERNAMENT_PROXY" = "y" ];
		then
			sudo bash -c "echo http_proxy=\"$SET_HTTP_PROXY\" >> /etc/environment"
			sudo bash -c "echo HTTP_PROXY=\"$SET_HTTP_PROXY\" >> /etc/environment"
			tmp=1
		fi
		export http_proxy=$SET_HTTP_PROXY
	fi
	if [ "`cat /etc/environment | grep https_proxy`" != "https_proxy=\"$SET_HTTPS_PROXY\"" ];
	then
		if [ "$PERNAMENT_PROXY" = "y" ];
		then
			sudo bash -c "echo https_proxy=\"$SET_HTTPS_PROXY\" >> /etc/environment"
			sudo bash -c "echo HTTPS_PROXY=\"$SET_HTTPS_PROXY\" >> /etc/environment"
			tmp=1
		fi
		export https_proxy=$SET_HTTPS_PROXY
	fi
	if [ "`cat /etc/environment | grep ftp_proxy`" != "ftp_proxy=\"$SET_FTP_PROXY\"" ];
	then
		if [ "$PERNAMENT_PROXY" = "y" ];
		then
			sudo bash -c "echo ftp_proxy=\"$SET_FTP_PROXY\" >> /etc/environment"
			sudo bash -c "echo FTP_PROXY=\"$SET_FTP_PROXY\" >> /etc/environment"
			tmp=1
		fi
		export ftp_proxy=$SET_FTP_PROXY
	fi
	if [ $tmp -eq 1 ]; then
		if [ "$IS_AUTO_INSTALL" != "y" ] && [ "$PERMANENT_PROXY" = "y" ]; then
			echo The System is update the proxy
			echo setting. After installing, please
			echo reboot this computer to apply the
			echo setting for other program.
			echo 
			pause
		fi
	fi
fi

echo Installing Build Package.
sudo apt-get upgrade -y && sudo apt-get update -y
if [ ! -f ${SAVE_STEP_PATH}/lib-apt-install ];
then
	# for all
	sudo apt-get install build-essential git -y

	# for json-c
	sudo apt-get install libjson0 libjson0-dev libjsoncpp0 libjsoncpp-dev libjson-glib-1.0-0 libjson-glib-dev -y
#	sudo apt-get isntall gcc clang libtool autoconf automake
	touch ${SAVE_STEP_PATH}/lib-apt-install
fi

#if [ ! -f ${SAVE_STEP_PATH}/lib-json-c ];
#then
#	cd $TEMP
#	git clone https://github.com/json-c/json-c.git
#	cd json-c
#	echo `pwd`
#	sudo sh autogen.sh
## wt* it should run 2 times for running autogen.sh properly
## test at 19/10/2014
#	sudo sh autogen.sh
#	sudo ./configure
#	sudo make
#	sudo make install
#	cd ../
##	sudo rm -r *
#	touch ${SAVE_STEP_PATH}/lib-json-c
#fi
echo Downloading CSS
cd $TEMP

echo "Installation completed"

exit 0
