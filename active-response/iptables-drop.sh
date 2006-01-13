#!/bin/sh
# Adds an IP to the iptables drop list
# Requirements: Linux with iptables installed
# Expect: srcip
# Author: Daniel B. Cid
# Last modified: Nov 11, 2005

UNAME=`uname`
IPTABLES="/sbin/iptables"
ACTION=$1
USER=$2
IP=$3

# We should only run on linux
if [ "X${UNAME}" != "XLinux" ]; then
   exit 0;
fi

# Checking if iptables is present
ls ${IPTABLES} >> /dev/null 2>&1
if [ $? != 0 ]; then
   IPTABLES="/usr/sbin/iptables"
   ls ${IPTABLES} >> /dev/null 2>&1
   if [ $? != 0 ]; then
      exit 0;
   fi
fi    
       
if [ "x${IP}" = "x" ]; then
   echo "$0: <username> <ip>" 
   exit 1;
fi


# Blocking IP
if [ "x${ACTION}" = "xadd" ]; then
   ${IPTABLES} -I INPUT -s ${IP} -j DROP
   ${IPTABLES} -I FORWARD -s ${IP} -j DROP
   exit 0;

# Removing IP block
elif [ "x${ACTION}" = "xdelete" ]; then
   ${IPTABLES} -D INPUT -s ${IP} -j DROP
   ${IPTABLES} -D FORWARD -s ${IP} -j DROP
   exit 0;

# Invalid action
else
   echo "$0: invalid action: ${ACTION}"
fi
       

exit 1;
