# https://www.virtualbox.org/manual/ch08.html

VBoxManage showvminfo < uuid | vmname > [--details] [--machinereadable] [--password-id] [--password]

VBoxManage showvminfo < uuid | vmname > <--log=index> [--password-id id] [--password file|-]


VBoxManage registervm <filename> --password file


VBoxManage unregistervm < uuid | vmname > [--delete] [--delete-all]


VBoxManage createvm <--name=name> [--basefolder=basefolder] [--default] [--groups=group-ID,...] [--ostype=ostype] [--register] [--uuid=uuid] [--cipher cipher] [--password-id password-id] [--password file]


VBoxManage modifyvm < uuid | vmname > [--name=name] [--groups= group [,group...] ] [--description=description] [--os-type=OS-type] [--icon-file=filename] [--memory=size-in-MB] [--page-fusion= on | off ] [--vram=size-in-MB] [--acpi= on | off ] [--ioapic= on | off ] [--hardware-uuid=UUID] [--cpus=CPU-count] [--cpu-hotplug= on | off ] [--plug-cpu=CPU-ID] [--unplug-cpu=CPU-ID] [--cpu-execution-cap=number] [--pae= on | off ] [--long-mode= on | off ] [--ibpb-on-vm-exit= on | off ] [--ibpb-on-vm-entry= on | off ] [--spec-ctrl= on | off ] [--l1d-flush-on-sched= on | off ] [--l1d-flush-on-vm-entry= on | off ] [--mds-clear-on-sched= on | off ] [--mds-clear-on-vm-entry= on | off ] [--cpu-profile= host | Intel 8086 | Intel 80286 | Intel 80386 ] [--hpet= on | off ] [--hwvirtex= on | off ] [--triple-fault-reset= on | off ] [--apic= on | off ] [--x2apic= on | off ] [--paravirt-provider= none | default | legacy | minimal | hyperv | kvm ] [--paravirt-debug= key=value [,key=value...] ] [--nested-paging= on | off ] [--large-pages= on | off ] [--vtx-vpid= on | off ] [--vtx-ux= on | off ] [--nested-hw-virt= on | off ] [--virt-vmsave-vmload= on | off ] [--accelerate-3d= on | off ] [--accelerate-2d-video= on | off ] [--chipset= ich9 | piix3 ] [--iommu= none | automatic | amd | intel ] [--tpm-type= none | 1.2 | 2.0 | host | swtpm ] [--tpm-location= location ] [--bios-logo-fade-in= on | off ] [--bios-logo-fade-out= on | off ] [--bios-logo-display-time=msec] [--bios-logo-image-path=pathname] [--bios-boot-menu= disabled | menuonly | messageandmenu ] [--bios-apic= disabled | apic | x2apic ] [--bios-system-time-offset=msec] [--bios-pxe-debug= on | off ] [--system-uuid-le= on | off ] [--bootX= none | floppy | dvd | disk | net ] [--rtc-use-utc= on | off ] [--graphicscontroller= none | vboxvga | vmsvga | vboxsvga ] [--snapshot-folder= default | pathname ] [--firmware= bios | efi | efi32 | efi64 ] [--guest-memory-balloon=size-in-MB] [--default-frontend= default | name ] [--vm-process-priority= default | flat | low | normal | high ]

VBoxManage modifyvm < uuid | vmname > [--nicN= none | null | nat | bridged | intnet | hostonly | hostonlynet | generic | natnetwork | cloud ] [--nic-typeN= Am79C970A | Am79C973 | 82540EM | 82543GC | 82545EM | virtio ] [--cable-connectedN= on | off ] [--nic-traceN= on | off ] [--nic-trace-fileN=filename] [--nic-propertyN=name= [value]] [--nic-speedN=kbps] [--nic-boot-prioN=priority] [--nic-promiscN= deny | allow-vms | allow-all ] [--nic-bandwidth-groupN= none | name ] [--bridge-adapterN= none | device-name ] [--cloud-networkN=network-name] [--host-only-adapterN= none | device-name ] [--host-only-netN=network-name] [--intnetN=network-name] [--nat-networkN=network-name] [--nic-generic-drvN=driver-name] [--mac-addressN= auto | MAC-address ]

VBoxManage modifyvm < uuid | vmname > [--nat-netN= network | default ] [--nat-pfN= [rule-name],tcp | udp,[host-IP],hostport,[guest-IP],guestport ] [--nat-pfN=delete=rule-name] [--nat-tftp-prefixN=prefix] [--nat-tftp-fileN=filename] [--nat-tftp-serverN=IP-address] [--nat-bind-ipN=IP-address] [--nat-dns-pass-domainN= on | off ] [--nat-dns-proxyN= on | off ] [--nat-dns-host-resolverN= on | off ] [--nat-localhostreachableN= on | off ] [--nat-settingsN=[mtu],[socksnd],[sockrcv],[tcpsnd],[tcprcv]] [--nat-alias-modeN= default | [log],[proxyonly],[sameports] ]

VBoxManage modifyvm < uuid | vmname > [--mouse= ps2 | usb | usbtablet | usbmultitouch | usbmtscreenpluspad ] [--keyboard= ps2 | usb ] [--uartN= off | IO-baseIRQ ] [--uart-modeN= disconnected | server pipe | client pipe | tcpserver port | tcpclient hostname:port | file filename | device-name ] [--uart-typeN= 16450 | 16550A | 16750 ] [--lpt-modeN=device-name] [--lptN= off | IO-baseIRQ ] [--audio-controller= ac97 | hda | sb16 ] [--audio-codec= stac9700 | ad1980 | stac9221 | sb16 ] [--audio-driver= none | default | null | dsound | was | oss | alsa | pulse | coreaudio ] [--audio-enabled= on | off ] [--audio-in= on | off ] [--audio-out= on | off ] [--clipboard-mode= disabled | hosttoguest | guesttohost | bidirectional ] [--drag-and-drop= disabled | hosttoguest | guesttohost | bidirectional ] [--monitor-count=number] [--usb-ehci= on | off ] [--usb-ohci= on | off ] [--usb-xhci= on | off ] [--usb-rename=old-namenew-name]

VBoxManage modifyvm < uuid | vmname > [--recording= on | off ] [--recording-screens= all | none | screen-ID[,screen-ID...] ] [--recording-file=filename] [--recording-max-size=MB] [--recording-max-time=msec] [--recording-opts= key=value[,key=value...] ] [--recording-video-fps=fps] [--recording-video-rate=rate] [--recording-video-res=widthheight]

VBoxManage modifyvm < uuid | vmname > [--vrde= on | off ] [--vrde-property=property-name= [property-value]] [--vrde-extpack= default | name ] [--vrde-port=port] [--vrde-address=hostip] [--vrde-auth-type= null | external | guest ] [--vrde-auth-library= default | name ] [--vrde-multi-con= on | off ] [--vrde-reuse-con= on | off ] [--vrde-video-channel= on | off ] [--vrde-video-channel-quality=percent]

VBoxManage modifyvm < uuid | vmname > [--teleporter= on | off ] [--teleporter-port=port] [--teleporter-address= address | empty ] [--teleporter-password=password] [--teleporter-password-file= filename | stdin ] [--cpuid-portability-level=level] [--cpuid-set=leaf [:subleaf]eax ebx ecx edx] [--cpuid-remove=leaf [:subleaf]] [--cpuid-remove-all]

VBoxManage modifyvm < uuid | vmname > [--tracing-enabled= on | off ] [--tracing-config=string] [--tracing-allow-vm-access= on | off ]

VBoxManage modifyvm < uuid | vmname > [--usb-card-reader= on | off ]

VBoxManage modifyvm < uuid | vmname > [--autostart-enabled= on | off ] [--autostart-delay=seconds]

VBoxManage modifyvm < uuid | vmname > [--guest-debug-provider= none | native | gdb | kd ] [--guest-debug-io-provider= none | tcp | udp | ipc ] [--guest-debug-address= IP-Address | path ] [--guest-debug-port=port]

VBoxManage modifyvm < uuid | vmname > [--pci-attach=host-PCI-address [@guest-PCI-bus-address]] [--pci-detach=host-PCI-address]

VBoxManage modifyvm < uuid | vmname > [--testing-enabled= on | off ] [--testing-mmio= on | off ] [--testing-cfg-dwordidx=value]


VBoxManage clonevm <vmname|uuid> [--basefolder=basefolder] [--groups=group,...] [ --mode=machine | --mode=machinechildren | --mode=all ] [--name=name] [--options=option,...] [--register] [--snapshot=snapshot-name] [--uuid=uuid]


VBoxManage movevm < uuid | vmname > [--type=basic] [--folder=folder-name]

VBoxManage startvm < uuid | vmname ...> [--putenv=name[=value]] [--type= [ gui | headless | sdl | separate ]] --password file --password-id password identifier

VBoxManage controlvm < uuid | vmname > poweroff

VBoxManage controlvm < uuid | vmname > savestate

VBoxManage controlvm < uuid | vmname > acpipowerbutton

VBoxManage controlvm < uuid | vmname > acpisleepbutton

VBoxManage controlvm < uuid | vmname > reboot

VBoxManage controlvm < uuid | vmname > shutdown [--force]

VBoxManage controlvm < uuid | vmname > keyboardputscancode <hex> [hex...]

VBoxManage controlvm < uuid | vmname > keyboardputstring <string> [string...]

VBoxManage controlvm < uuid | vmname > keyboardputfile <filename>

VBoxManage controlvm < uuid | vmname > setlinkstateN < on | off >

VBoxManage controlvm < uuid | vmname > nicN < null | nat | bridged | intnet | hostonly | generic | natnetwork > [device-name]

VBoxManage controlvm < uuid | vmname > nictraceN < on | off >

VBoxManage controlvm < uuid | vmname > nictracefileN <filename>

VBoxManage controlvm < uuid | vmname > nicpropertyN <prop-name=prop-value>

VBoxManage controlvm < uuid | vmname > nicpromiscN < deny | allow-vms | allow-all >

VBoxManage controlvm < uuid | vmname > natpfN < [rulename] ,tcp | udp, [host-IP], hostport, [guest-IP], guestport >

VBoxManage controlvm < uuid | vmname > natpfN delete <rulename>

VBoxManage controlvm < uuid | vmname > guestmemoryballoon <balloon-size>

VBoxManage controlvm < uuid | vmname > usbattach < uuid | address > [--capturefile=filename]

VBoxManage controlvm < uuid | vmname > usbdetach < uuid | address >

VBoxManage controlvm < uuid | vmname > audioin < on | off >

VBoxManage controlvm < uuid | vmname > audioout < on | off >

VBoxManage controlvm < uuid | vmname > clipboard filetransfers < on | off >

VBoxManage controlvm < uuid | vmname > draganddrop < disabled | hosttoguest | guesttohost | bidirectional >

VBoxManage controlvm < uuid | vmname > vrde < on | off >

VBoxManage controlvm < uuid | vmname > vrdeport <port>

VBoxManage controlvm < uuid | vmname > vrdeproperty <prop-name=prop-value>

VBoxManage controlvm < uuid | vmname > vrdevideochannelquality <percentage>

VBoxManage controlvm < uuid | vmname > setvideomodehint <xres> <yres> <bpp> [[display] [ enabled:yes | no | [x-origin y-origin]]]

VBoxManage controlvm < uuid | vmname > setscreenlayout <display> < on | primary x-origin y-origin x-resolution y-resolution bpp | off >

VBoxManage controlvm < uuid | vmname > screenshotpng <filename> [display]

VBoxManage controlvm < uuid | vmname > recording < on | off >

VBoxManage controlvm < uuid | vmname > recording screens < all | none | screen-ID[,screen-ID...] >

VBoxManage controlvm < uuid | vmname > recording filename <filename>

VBoxManage controlvm < uuid | vmname > recording videores <widthxheight>

VBoxManage controlvm < uuid | vmname > recording videorate <rate>

VBoxManage controlvm < uuid | vmname > recording videofps <fps>

VBoxManage controlvm < uuid | vmname > recording maxtime <sec>

VBoxManage controlvm < uuid | vmname > recording maxfilesize <MB>

VBoxManage controlvm < uuid | vmname > setcredentials <username> --passwordfile= < filename | password > <domain-name> --allowlocallogon= < yes | no >

VBoxManage controlvm < uuid | vmname > teleport <--host=host-name> <--port=port-name> [--maxdowntime=msec] [ --passwordfile=filename | --password=password ]

VBoxManage controlvm < uuid | vmname > plugcpu <ID>

VBoxManage controlvm < uuid | vmname > unplugcpu <ID>

VBoxManage controlvm < uuid | vmname > cpuexecutioncap <num>

VBoxManage controlvm < uuid | vmname > vm-process-priority < default | flat | low | normal | high >

VBoxManage controlvm < uuid | vmname > webcam attach [pathname [settings]]

VBoxManage controlvm < uuid | vmname > webcam detach [pathname]

VBoxManage controlvm < uuid | vmname > webcam list

VBoxManage controlvm < uuid | vmname > addencpassword <ID> < password-file | - > [--removeonsuspend= yes | no ]

VBoxManage controlvm < uuid | vmname > removeencpassword <ID>

VBoxManage controlvm < uuid | vmname > removeallencpasswords

VBoxManage controlvm < uuid | vmname > changeuartmodeN disconnected | server pipe-name | client pipe-name | tcpserver port | tcpclient hostname:port | file filename | device-name

VBoxManage controlvm < uuid | vmname > autostart-enabledN on | off

VBoxManage controlvm < uuid | vmname > autostart-delayseconds

VBoxManage unattended detect <--iso=install-iso> [--machine-readable]

VBoxManage unattended install <uuid|vmname> <--iso=install-iso> [--user=login] [--password=password] [--password-file=file] [--full-user-name=name] [--key=product-key] [--install-additions] [--no-install-additions] [--additions-iso=add-iso] [--install-txs] [--no-install-txs] [--validation-kit-iso=testing-iso] [--locale=ll_CC] [--country=CC] [--time-zone=tz] [--hostname=fqdn] [--package-selection-adjustment=keyword] [--dry-run] [--auxiliary-base-path=path] [--image-index=number] [--script-template=file] [--post-install-template=file] [--post-install-command=command] [--extra-install-kernel-parameters=params] [--language=lang] [--start-vm=session-type]


VBoxManage discardstate < uuid | vmname >


VBoxManage adoptstate < uuid | vmname > <state-filename>


VBoxManage snapshot <uuid|vmname>

VBoxManage snapshot <uuid|vmname> take <snapshot-name> [--description=description] [--live] [--uniquename Number,Timestamp,Space,Force]

VBoxManage snapshot <uuid|vmname> delete <snapshot-name>

VBoxManage snapshot <uuid|vmname> restore <snapshot-name>

VBoxManage snapshot <uuid|vmname> restorecurrent

VBoxManage snapshot <uuid|vmname> edit < snapshot-name | --current > [--description=description] [--name=new-name]

VBoxManage snapshot <uuid|vmname> list [[--details] | [--machinereadable]]

VBoxManage snapshot <uuid|vmname> showvminfo <snapshot-name>


VBoxManage closemedium [ disk | dvd | floppy ] < uuid | filename > [--delete]


VBoxManage storageattach < uuid | vmname > <--storagectl=name> [--bandwidthgroup= name | none ] [--comment=text] [--device=number] [--discard= on | off ] [--encodedlun=lun] [--forceunmount] [--hotpluggable= on | off ] [--initiator=initiator] [--intnet] [--lun=lun] [--medium= none | emptydrive | additions | uuid | filename | host:drive | iscsi ] [--mtype= normal | writethrough | immutable | shareable | readonly | multiattach ] [--nonrotational= on | off ] [--passthrough= on | off ] [--passwordfile=file] [--password=password] [--port=number] [--server= name | ip ] [--setparentuuid=uuid] [--setuuid=uuid] [--target=target] [--tempeject= on | off ] [--tport=port] [--type= dvddrive | fdd | hdd ] [--username=username]


VBoxManage storagectl < uuid | vmname > <--name=controller-name> [--add= floppy | ide | pcie | sas | sata | scsi | usb ] [--controller= BusLogic | I82078 | ICH6 | IntelAhci | LSILogic | LSILogicSAS | NVMe | PIIX3 | PIIX4 | USB | VirtIO ] [--bootable= on | off ] [--hostiocache= on | off ] [--portcount=count] [--remove] [--rename=new-controller-name]

VBoxManage showmediuminfo [ disk | dvd | floppy ] < uuid | filename >


VBoxManage createmedium [ disk | dvd | floppy ] <--filename=filename> [ --size=megabytes | --sizebyte=bytes ] [--diffparent= UUID | filename ] [--format= VDI | VMDK | VHD ] [--variant Standard,Fixed,Split2G,Stream,ESX,Formatted,RawDisk] --property name=value... --property-file name=/path/to/file/with/value...


VBoxManage modifymedium [ disk | dvd | floppy ] < uuid | filename > [--autoreset=on | off] [--compact] [--description=description] [--move=pathname] [--property=name=[value]] [--resize=megabytes | --resizebyte=bytes] [--setlocation=pathname] [--type=normal | writethrough | immutable | shareable | readonly | multiattach]


VBoxManage clonemedium < uuid | source-medium > < uuid | target-medium > [ disk | dvd | floppy ] [--existing] [--format= VDI | VMDK | VHD | RAW | other ] [--variant=Standard,Fixed,Split2G,Stream,ESX]


VBoxManage mediumproperty [ disk | dvd | floppy ] set < uuid | filename > <property-name> <property-value>

VBoxManage mediumproperty [ disk | dvd | floppy ] get < uuid | filename > <property-name>

VBoxManage mediumproperty [ disk | dvd | floppy ] delete < uuid | filename > <property-name>

VBoxManage mediumio < --disk=uuid|filename | --dvd=uuid|filename | --floppy=uuid|filename > [--password-file=-|filename] formatfat [--quick]

VBoxManage mediumio < --disk=uuid|filename | --dvd=uuid|filename | --floppy=uuid|filename > [--password-file=-|filename] cat [--hex] [--offset=byte-offset] [--size=bytes] [--output=-|filename]

VBoxManage mediumio < --disk=uuid|filename | --dvd=uuid|filename | --floppy=uuid|filename > [--password-file=-|filename] stream [--format=image-format] [--variant=image-variant] [--output=-|filename]

VBoxManage sharedfolder add < uuid | vmname > <--name=name> <--hostpath=hostpath> [--readonly] [--transient] [--automount] [--auto-mount-point=path]

VBoxManage sharedfolder remove < uuid | vmname > <--name=name> [--transient]


VBoxManage guestproperty get < uuid | vmname > <property-name> [--verbose]

VBoxManage guestproperty enumerate < uuid | vmname > [--no-timestamp] [--no-flags] [--relative] [--old-format] [patterns...]

VBoxManage guestproperty set < uuid | vmname > <property-name> [property-value [--flags=flags]]

VBoxManage guestproperty unset < uuid | vmname > <property-name>

VBoxManage guestproperty wait < uuid | vmname > <patterns> [--timeout=msec] [--fail-on-timeout]


VBoxManage guestcontrol < uuid | vmname > run [--arg0=argument 0] [--domain=domainname] [--dos2unix] [--exe=filename] [--ignore-orphaned-processes] [ --no-wait-stderr | --wait-stderr ] [ --no-wait-stdout | --wait-stdout ] [ --passwordfile=password-file | --password=password ] [--profile] [--putenv=var-name=[value]] [--quiet] [--timeout=msec] [--unix2dos] [--unquoted-args] [--username=username] [--verbose] <-- [argument...]>

VBoxManage guestcontrol < uuid | vmname > start [--arg0=argument 0] [--domain=domainname] [--exe=filename] [--ignore-orphaned-processes] [ --passwordfile=password-file | --password=password ] [--profile] [--putenv=var-name=[value]] [--quiet] [--timeout=msec] [--unquoted-args] [--username=username] [--verbose] <-- [argument...]>

VBoxManage guestcontrol < uuid | vmname > copyfrom [--dereference] [--domain=domainname] [ --passwordfile=password-file | --password=password ] [--quiet] [--no-replace] [--recursive] [--target-directory=host-destination-dir] [--update] [--username=username] [--verbose] <guest-source0> guest-source1 [...] <host-destination>

VBoxManage guestcontrol < uuid | vmname > copyto [--dereference] [--domain=domainname] [ --passwordfile=password-file | --password=password ] [--quiet] [--no-replace] [--recursive] [--target-directory=guest-destination-dir] [--update] [--username=username] [--verbose] <host-source0> host-source1 [...]

VBoxManage guestcontrol < uuid | vmname > mkdir [--domain=domainname] [--mode=mode] [--parents] [ --passwordfile=password-file | --password=password ] [--quiet] [--username=username] [--verbose] <guest-directory...>

VBoxManage guestcontrol < uuid | vmname > rmdir [--domain=domainname] [ --passwordfile=password-file | --password=password ] [--quiet] [--recursive] [--username=username] [--verbose] <guest-directory...>

VBoxManage guestcontrol < uuid | vmname > rm [--domain=domainname] [--force] [ --passwordfile=password-file | --password=password ] [--quiet] [--username=username] [--verbose] <guest-directory...>

VBoxManage guestcontrol < uuid | vmname > mv [--domain=domainname] [ --passwordfile=password-file | --password=password ] [--quiet] [--username=username] [--verbose] <source...> <destination-directory>

VBoxManage guestcontrol < uuid | vmname > mktemp [--directory] [--domain=domainname] [--mode=mode] [ --passwordfile=password-file | --password=password ] [--quiet] [--secure] [--tmpdir=directory-name] [--username=username] [--verbose] <template-name>

VBoxManage guestcontrol < uuid | vmname > stat [--domain=domainname] [ --passwordfile=password-file | --password=password ] [--quiet] [--username=username] [--verbose] <filename>

VBoxManage guestcontrol < uuid | vmname > list < all | files | processes | sessions > [--quiet] [--verbose]

VBoxManage guestcontrol < uuid | vmname > closeprocess [ --session-id=ID | --session-name=name-or-pattern ] [--quiet] [--verbose] <PID...>

VBoxManage guestcontrol < uuid | vmname > closesession [ --all | --session-id=ID | --session-name=name-or-pattern ] [--quiet] [--verbose]

VBoxManage guestcontrol < uuid | vmname > updatega [--quiet] [--verbose] [--source=guest-additions.ISO] [--wait-start] [-- [argument...]]

VBoxManage guestcontrol < uuid | vmname > watch [--quiet] [--verbose]

VBoxManage hostonlyif ipconfig <ifname> [ --dhcp | --ip=IPv4-address [--netmask=IPv4-netmask] | --ipv6=IPv6-address [--netmasklengthv6=length] ]

VBoxManage hostonlyif create

VBoxManage hostonlyif remove <ifname>


VBoxManage hostonlynet add <--name=netname> [--id=netid] <--netmask=mask> <--lower-ip=address> <--upper-ip=address> [ --enable | --disable ]

VBoxManage hostonlynet modify < --name=netname | --id=netid > [--lower-ip=address] [--upper-ip=address] [--netmask=mask] [ --enable | --disable ]

VBoxManage hostonlynet remove < --name=netname | --id=netid >