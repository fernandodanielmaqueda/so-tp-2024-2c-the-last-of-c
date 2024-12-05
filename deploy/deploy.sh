#!/bin/sh

set -x

VBoxManage='/c/Program Files/Oracle/VirtualBox/VBoxManage.exe'
VMName='Ubuntu Server 6.1'
#--type headless

"$VBoxManage" startvm "$VMName"

# "$VBoxManage" guestcontrol "$VMName" run --image "./vm.sh" --username 'utnso' --password 'utnso' --wait-exit --wait-stdout --wait-stderr