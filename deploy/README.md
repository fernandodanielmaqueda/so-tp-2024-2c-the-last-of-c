# Notas de Deploy SS.OO

- Algunas de las cosas que se listan también sirven para instalar el entorno de trabajo, no todo es exclusivo para el deploy para la presentación del TP.
- Las máquinas de la facultad usan Windows, y los alumnos NO disponen permisos de administrador: no se pueden instalar programas, no se pueden compartir carpetas por SMB (Samba), etc.

### TODO
- Evaluar cómo hacer para pasar el repositorio y los scripts de deploy lo más fácil posible a las máquinas de la facultad (.iso, .vhd)
- Editar el archivo /home/utnso/base.sh acordemente
- Hacer un script que instale automáticamente las extensiones de VSCode
-----------------------------

# (SÓLO en el Deploy) De antemano

## A. Leer documentación

- https://www.utnso.com.ar/
- https://docs.utnso.com.ar/primeros-pasos/normas-tp
- https://docs.utnso.com.ar/guias/herramientas/deploy
- https://docs.utnso.com.ar/primeros-pasos/tp0

>Si tuviéramos que resumir en tres cosas, lo que deberían hacer para poder llegar a una entrega con la seguridad de que van a aprobar sería:
>
>- Probar el TP
>- Practicar el despliegue (deploy)
>- Probar el TP y practicar el despliegue (deploy)
>
>Para esto pueden ir a los laboratorios de Medrano (que están abiertos de lunes a viernes de 10 a 21hs y sábados de 10 a 18hs)
>o, de no ser posible, pueden utilizar las VMs server desde sus casas de la misma forma que en el TP0. 

## B. Preparar las configs en el repositorio grupal

> Para conectar los módulos del TP entre sí es recomendable usar números de puerto por encima de 1024, ya que es poco probable que estén siendo usados por otro proceso (ej: 8000, 8001, etc).

## C. Anotarse en el Sistema de Inscripciones

- https://inscripciones.utnso.com.ar/

>Es muy importante que se anoten para que los podamos evaluar, ya que grupo que no esté anotado por más que se presenten en el laboratorio no va a ser evaluado.
>
>Cualquier integrante del grupo podrá ingresar al Sistema de Inscripciones y deberá indicar la franja horaria en que el grupo pueda presentarse.
>
>El grupo luego recibirá la fecha y horario estimado de evaluación. Los mismos se asignarán en base a la cantidad de grupos inscriptos. La inscripción cerrará el día anterior a la fecha de entrega a las 20:00hs.
>
>Por último les pedimos que si deciden no presentarse a esta entrega nos avisen por mail a fin de poder organizarnos de la mejor manera.

## D. Generar las credenciales de GitHub

- Para claves PAT (Personal Access Token)
	- Fine-grained tokens
		- https://github.com/settings/tokens?type=beta
	- Tokens (classic)
		- https://github.com/settings/tokens
			- Los scopes (alcances) mínimos requeridos son: `repo`, `read:org` y `workflow`.

## E. Llevarse anotadas las credenciales de GitHub

- Ya sea en un papel, en el celular o en un pendrive

-----------------------------

# (SÓLO en el Deploy) Anunciarse al llegar *TODOS* al laboratorio

**Laboratorio de Sistemas UTN FRBA: 3er piso, ~Aula 317, Sede Medrano**

>El día de la evaluación, el grupo deberá notificar su llegada a los ayudantes 
>
>El grupo deberá concurrir el día de la evaluación en el horario asignado, teniendo que anunciar su llegada a alguno de los ayudantes presentes.
>
>Es importante que al momento de anunciarse estén todos los integrantes del grupo.
>
>En caso de que pasen 5 minutos del horario asignado y el grupo no se haya anunciado se asumirá que el grupo no se presenta.

-----------------------------

# (SÓLO en el Deploy) Una vez con las máquinas del laboratorio

>Si el trabajo práctico no puede correr en más de una máquina no se iniciará la evaluación y se dará por desaprobada la entrega.
>
>Al momento de iniciar el despliegue del TP según lo indique el ayudante evaluador, el grupo dispondrá de 10 minutos para estar en condiciones de ser evaluado, en caso de que transcurran los 10 minutos y no se pueda iniciar la evaluación, el ayudante evaluador podrá solicitarles que se retiren del laboratorio, dando por finalizada la evaluación con resultado desaprobado.
>
>Los grupos contarán con una unica instancia de evaluación por fecha de entrega, es decir, que ante un error no resoluble en el momento en las pruebas, la entrega se considerará desaprobada.

-----------------------------

## 1. (SÓLO en el Deploy) Prender e iniciar sesión en una computadora

- **Usuario**: `alumno`
- **Contraseña**: `alumno`

-----------------------------

## 2. (NO en el Deploy) Abrir un navegador web

Por ejemplo: `Google Chrome`, etc.

## 3. (NO en el Deploy) Loguearse en la cuenta de GitHub

- https://github.com/

## 4. (NO en el Deploy) Acceder a este repositorio grupal

- https://github.com/sisoputnfrba/tp-2024-2c-so

## 5. (NO en el Deploy) Descargar, instalar y abrir el Git Bash *PORTABLE*

- https://git-scm.com/download/win
	- 32-bits:
		- https://github.com/git-for-windows/git/releases/download/v2.45.2.windows.1/PortableGit-2.45.2-32-bit.7z.exe
	- 64-bits:
		- https://github.com/git-for-windows/git/releases/download/v2.45.2.windows.1/PortableGit-2.45.2-64-bit.7z.exe 

-----------------------------

## 6. (NO en el Deploy) Iniciar sesión en el Git de Windows
```bash
git config --global user.email 'ejemplo@frba.utn.edu.ar'
git config --global user.name 'Nombre y Apellido(s)'
```

-----------------------------

## 7. (NO en el Deploy) Clonar este repositorio en Windows

1. Desactivar la conversión automática a CRLF al clonar el repositorio
```bash
git config --global core.autocrlf false
```

1. Asegurar que todos los archivos del repositorio utilicen LF
```bash
git config --global core.eol lf
```

2. Desactivar la compresión
```bash
git config --global core.compression 0
```

3. Incrementar el tamaño del buffer
Se puede probar con distintos valores: 150MB (157286400), 500MB (524288000)
```bash
git config --global http.postBuffer 524288000
git config --global http.postBuffer 157286400
```

4. Clonar (sin --recurse-submodules)
```bash
cd ~ ; git clone --depth 1 --branch main --single-branch --no-tags https://github.com/sisoputnfrba/tp-2024-2c-so
```

> Debería ser equivalente a:
```bash
cd /c/Users/alumno ; git clone --depth 1 --branch main --single-branch --no-tags https://github.com/sisoputnfrba/tp-2024-2c-so
```

5. Moverse a la rama que corresponda
```bash
git checkout main
```

### En caso de que aparezca este error:

[![.NETFRAMEWORK_ERROR](https://i.sstatic.net/O9L6E.png)]()

Seleccionar `No`: Como no tenemos permisos de administrador en las máquinas de la facultad, no podemos instalar esa dependencia
(.NET Framework 4.7.2) y por ende no podemos utilizar el Administrador de Credenciales de Git
(**Git Credential Manager Core**)

Las alternativas son:

A) Si para acceder a tu cuenta de Git usás PAT (Personal Access Token):
- Usá el Administrador de Credenciales de Windows (**wincred**)
```bash
git config --unset-all credential.helper && git config credential.helper wincred
```
- Cuando eventualmente se te solicite tu nombre de usuario, ingresalo.
- Cuando eventualmente te solicite tu contraseña, ingresá tu PAT (Personal Access Token) que generaste.

B) Si para acceder a tu cuenta de Git usás clave SSH:
- Cambiar a SSH:
```bash
git remote set-url origin git@github.com:sisoputnfrba/tp-2024-2c-so.git
```

- Eventualmente deberás ingresar tu clave pública SSH generada.

-----------------------------

## 8. (NO en el Deploy) Clonarse los submódulos del repositorio en Windows
```bash
cd tp-2024-2c-so ; git submodule update --init --recursive --depth 1
```

-----------------------------

## 9. (NO en el Deploy) Ejecutar los Scripts Automáticos de Windows

TODO

-----------------------------

## 10. (NO en el Deploy) Descargar VirtualBox

- https://www.virtualbox.org/wiki/Downloads

-----------------------------

## 11. (NO en el Deploy) Descargar la VM de Ubuntu Server

Página oficial
- https://docs.utnso.com.ar/recursos/vms

Links de descarga
- https://drive.google.com/drive/folders/1Pn1SveTGkEVfcc7dYAr1Wc10ftEe8E0J
	- https://drive.google.com/file/d/16lUW1fRvoitkUiUN58mwJrmyAVezcOdx/view
	- https://drive.google.com/file/d/16lUW1fRvoitkUiUN58mwJrmyAVezcOdx/view?usp=drive_link
	- https://drive.google.com/file/d/16lUW1fRvoitkUiUN58mwJrmyAVezcOdx/view?usp=sharing
		- https://drive.google.com/uc?export=download&id=16lUW1fRvoitkUiUN58mwJrmyAVezcOdx
		- https://drive.google.com/u/0/uc?id=16lUW1fRvoitkUiUN58mwJrmyAVezcOdx&export=download
		- https://drive.google.com/uc?id=16lUW1fRvoitkUiUN58mwJrmyAVezcOdx
	
Generadores de links de descarga de Google Drive:
	- https://www.wonderplugin.com/online-tools/google-drive-direct-link-generator/
	- https://www.techyleaf.in/google-drive-direct-link-generator/
	- https://www.innateblogger.com/p/google-drive-direct-link-generator.html
	- https://chromewebstore.google.com/detail/google-drive-direct-link/ebfajbnnlkjdogmocghbakjbncbgiljb?pli=1

Gestores de descargas (Windows)
- https://www.freedownloadmanager.org/es/download.htm
- https://ugetdm.com/downloads/windows/#downloadable-packages
- https://github.com/setvisible/ArrowDL

Gestores de descargas (Linux)
- https://chemicloud.com/blog/download-google-drive-files-using-wget/
- https://medium.com/@acpanjan/download-google-drive-files-using-wget-3c2c025a8b99
- https://stackoverflow.com/questions/25010369/wget-curl-large-file-from-google-drive
- https://superuser.com/questions/1518582/how-to-use-wget-to-download-a-file-stored-in-google-drive-without-making-publicl

-----------------------------

## 12. (NO en el Deploy) Revisar características de Windows

- Desactivar `Plataforma de Hipervisor de Windows`

-----------------------------

## 13. (NO en el Deploy) Configuración de la VM (Ubuntu Server) en VirtualBox

- General
	- Básico
		- `Nombre`: SO 2022 Actualizada
		- `Tipo`: Linux
		- `Versión`: Ubuntu (64-bit)
	- Avanzado
		- `Carpeta de instantáneas`: C:\Users\alumno\VirtualBox VMs\SO 2022 Actualizada\Snapshots
		- `Portapapeles compartido`: Inhabilitado
		- `Arrastrar y soltar`: Inhabilitado
	- Descripción
	- Cifrado de disco
		- [ ] Habilitar cifrado de disco

- Sistema
	- Placa base
		- `Memoria base`: 2048 MB
		- `Orden de arranque`: Óptica - Disco Duro
		- `Chipset`: PIIX3
		- `TPM`: Ninguno
		- `Dispositivo apuntador`: Tableta USB
		- `Características extendidas`:
			- [X] Habilitar I/O APIC
			- [X] Habilitar reloj hardware en tiempo UTC
			- [ ] Habilitar EFI (sólo SO especiales)
			- [ ] Habilitar Secure Boot
	- Procesador
		- `Procesadores`: 1
		- `Límite de ejecución`: 100%
		- `Características extendidas`:
			- [ ] Habilitar PAE/NX
			- [ ] Habilitar VT-x/AMD-V anidado
	- Aceleración
		- `Interfaz de paravirtualización`: Predeterminado
		- Hardware de virtualización:
			- [X] Habilitar paginación anidada

- Pantalla
	- Pantalla
		- `Memoria de video`: 16 MB
		- `Número de monitores`: 1
		- `Factor de escalado`: Todos los monitores | 100%
		- `Controlador gráfico`: VBoxVGA
		- Caracteristicas extendidas:
			- [ ] Habilitar aceleración 3D
	- Pantalla remota
		- [ ] Habilitar servidor
	- Grabación
		- [ ] Habilitar grabación

- Almacenamiento

- Audio
	- [ ] Habilitar audio

		- `Controlador de audio anfitrión`: Predeterminado
		- `Controlador de audio`: ICH 1C97
		- Características extendidas:
			- [X] Habilitar salida de audio
			- [ ] Habilitar entrada de audio

- Red
	- Adaptador 1
		- [ ] Habilitar adaptador de red
			- `Conectado a`: Adaptador puente
			- `Nombre`: ...
			- \> Avanzado
				- `Tipo de adaptador`: ...
				- `Modo promiscuo`: Denegar
				- `Dirección MAC`: ...
				- [X] Cable conectado
	- Adaptador 2
	- Adaptador 3
	- Adaptador 4

- Puertos serie
	- Puerto 1
		- [ ] Habilitar puerto serie
	- Puerto 2
	- Puerto 3
	- Puerto 4

- USB
	- [X] Habilitar controlador USB
		- (O) Controlador USB 1.1 (OHCI)
		- (O) Controlador USB 2.0 (OHCI + EHCI)
		- (O) Controlador USB 3.0 (xHCI)
		- Filtros de dispositivos USB

- Carpetas compartidas
	- v Carpetas de la máquina
		- `Nombre`: Compartida | `Ruta`: C:\Users\alumno\VirtualBox VMs\Compartida | `Acceso`: Completo | `Automontar`: Sí | `En`:
	- v Carpetas transitorias

- Interfaz de usuario
	- [X] Archivo
	- [X] Máquina
	- [X] Ver
	- [X] Entrada
	- [X] Dispositivos
	- [X] Ayuda
	- [X] .
	- `Estado visual`: Normal (ventana)
	- Minibarra de herramientas:
		- [X] Mostrar en pantalla completa/fluído
		- [ ] Mostrar en la parte superior de la pantalla
	- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .
		- [X] .

-----------------------------
## 14. (SÓLO en el Deploy) Restaurar la VM Server al último Snapshot (Instantánea) provisto por la Cátedra

Es en caso de que otro grupo la haya usado antes.

`Instantánea` > `Restaurar`

- **SO 2022 Actualizada** *(Base)*

-----------------------------
## 15. (NO en el Deploy) Hacer un Snapshot (Instantánea) de la VM Server

`Instantánea` > `Tomar`

-----------------------------
## 16. Iniciar la VM Server

- **SO 2022 Actualizada**

# En la VM de Ubuntu Server

- **utnso login**: `utnso`
- **Password**: `utnso`

-----------------------------
## 17. (SÓLO en el Deploy) Pasar la IP (de la VM) al resto de los integrantes del grupo

Ya sea enviando un mensaje de texto desde un celular, o anotando en un papel

- Para mostrar la IP de la Guest VM Server:
```bash
ifconfig
```

[![ifconfig](https://docs.utnso.com.ar/img/guias/consola/bash-ifconfig.png)]()

-----------------------------
## 18. Actualizar el índice de paquetes local en la VM

```bash
sudo apt update
```

-----------------------------

## 19. (NO en el Deploy) (Opcional) GRUB (para configurar el tamaño de pantalla FIJO de la VM)

1. Backupear el archivo `/etc/default/grub` original
```bash
sudo cp /etc/default/grub /etc/default/grub.original
```

2. Editar el archivo `/etc/default/grub`
```bash
sudo vi /etc/default/grub
```

> Agregar al final la siguiente línea:
```output
GRUB_GFXPAYLOAD_LINUX=800x600
```

> Modificar la siguiente línea:
```output
GRUB_CMDLINE_LINUX_DEFAULT=""
```
> a:
```output
GRUB_CMDLINE_LINUX_DEFAULT="nomodeset"
```

3. Actualizar el gestor de arranque GRUB

```bash
sudo update-grub
```

4. Reiniciar la VM
```bash
init 6
```

-----------------------------

## 20. (NO en el Deploy) Instalar VirtualBox Guest Additions (para las carpetas compartidas)

1. Iniciada la VM, ir a: `Dispositivos` > `Insertar imagen de CD de las Guest Additions`

2. Crear el directorio donde se montará el CD de las Guest Additions:
```bash
sudo mkdir /mnt/cdrom
```

3. Montar el CD de las Guest Additions en dicho directorio creado
```bash
sudo mount /dev/cdrom /mnt/cdrom
```

4. Ejecutar el instalador de las Guest Additions para Linux
```bash
sudo /mnt/cdrom/VBoxLinuxAdditions.run
```

5. Por si acaso, también agregar el user actual (`utnso`) al grupo de usuarios `vboxsf` para tener permisos de lectura y escritura en las carpetas compartidas:
```bash
sudo adduser $USER vboxsf
sudo usermod -aG vboxsf $USER
```

-----------------------------

## 21. (NO en el Deploy) Montar una carpeta compartida en la VM

### (NO en el Deploy) Alternativa 1: Montar una carpeta compartida usando vboxsf (VirtualBox Shared Folders)

1. Crear el directorio donde montaremos la carpeta compartida en la VM
```bash
mkdir /home/utnso/vboxsfCompartida
```

2. Montar manualmente la carpeta compartida en la VM
```bash
sudo mount -t vboxsf Compartida /home/utnso/vboxsfCompartida -o rw,exec,uid=1000,gid=1000
```

3. Verificar que la carpeta compartida se haya montado correctamente en la VM
```bash
ls /home/utnso/vboxsfCompartida
```

4. Para que la carpeta compartida se monte automáticamente cada vez que iniciemos la VM, editar el archivo `/etc/fstab`:
```bash
sudo vi /etc/fstab
```

> Agregarle la siguiente línea al final de dicho archivo.
> Nótese el uso de tabulaciones en lugar de espacios para separar las columnas de la línea.
```output
Compartida	/home/utnso/vboxsfCompartida	vboxsf	rw,exec,uid=1000,gid=1000	0	0
```

5. Para arrancar el servicio de carpetas compartidas de VirtualBox cada vez que iniciemos la VM, editar el archivo `/etc/modules`:
```bash
sudo vi /etc/modules
```

> Agregarle la siguiente línea al final de dicho archivo.
```output
vboxsf
```

### Nota sobre las carpetas compartidas de VirtualBox

- No se puede ejecutar un programa más de una vez en simultáneo (múltiples instancias de un programa)
- No se pueden depurar con `gdb` los ejecutables si estos están dentro de las carpetas compartidas (sólo se puede hasta cargar los símbolos de depuración del mismo)

Esto es por una limitación de los permisos de ejecución que tiene el filesystem de VirtualBox (`vboxsf`):
- Mientras un programa está en ejecución, se quitan los permisos de ejecución y de escritura sobre el mismo (`x` y `w`)
- Los permisos NO se pueden modificar con `chmod`.
- chown tampoco tiene efecto.
- Sucede independientemente del `gid` y del `pid`, seas usuario `utnso`, `root` y/o parte del grupo `vboxsf`

Una forma rápida para sobrellevarlo es copiar los archivos necesarios a ejecutar de la carpeta compartida a cualquier carpeta local de la VM y ejecutarlos desde ahí

-----------------------------

### (NO en el Deploy) Alternativa 2: Montar una carpeta compartida usando Samba (SMB)

> Este método no tiene la limitación anteriormente mencionada.

En la VM Ubuntu Server:

1. Descargar e instalar `cifs-utils`
```bash
sudo apt install cifs-utils
```

2. Crear el directorio donde montaremos la carpeta compartida en la VM
```bash
mkdir /home/utnso/smbCompartida
```

3. Montar manualmente la carpeta compartida en la VM
```bash
sudo mount -t cifs //alumno/NombreCarpetaCompartida /home/utnso/smbCompartida -o username=alumno,password=alumno,vers=3.0,file_mode=0777,dir_mode=0777,uid=1000,gid=1000
```

4. Verificar que la carpeta compartida se haya montado correctamente en la VM
```bash
ls /home/utnso/smbCompartida
```

5. Para que la carpeta compartida se monte automáticamente cada vez que iniciemos la VM, editar el archivo `/etc/fstab`:
```bash
sudo vi /etc/fstab
```

> Agregarle la siguiente línea al final de dicho archivo.
```output
//alumno/NombreCarpetaCompartida /home/utnso/smbCompartida cifs username=alumno,password=alumno,vers=3.0,file_mode=0777,dir_mode=0777 0 0
```

`Nota`: `vers=3.0` es para indicar la versión de Samba (SMB) utilizada. Puede cambiarse a 2.0, por ejemplo, de ser necesario

-----------------------------

### (NO en el Deploy) Alternativa 3: Montar una carpeta compartida usando ...

- NFS
- sshfs (ssh filesystem): sudo apt-get install sshfs
- gdbserver

-----------------------------

## 22. (NO en el Deploy) Configurar SSH en la VM

1. Descargar e instalar `openssh-server`
```bash
sudo apt install openssh-server -y
```

2. Asegurarse de que `ssh.service` esté activo (ejecutando)
```bash
sudo systemctl status ssh
sudo systemctl enable ssh --now
sudo systemctl status ssh
```

> La salida debería ser similar a:
```output
● ssh.service - OpenBSD Secure Shell server
     Loaded: loaded (/lib/systemd/system/ssh.service; enabled; vendor preset: enabled)
     Active: active (running) since Wed yyyy-mm-dd hh:mm:ss UTC; #h ago
       Docs: man:sshd(8)
             man:sshd_config(5)
    Process: 733 ExecStartPre=/usr/sbin/sshd -t (code=exited, status=0/SUCCESS)
   Main PID: 778 (sshd)
      Tasks: 1 (limit: 2220)
     Memory: 7.7M
        CPU: 125ms
     CGroup: /system.slice/ssh.service
             └─778 "sshd: /usr/sbin/sshd -D [listener] 0 of 10-100 startups"
```

3. Asegurarse de que el firewall esté funcionando y de que permita ssh
```bash
sudo ufw status verbose
sudo ufw allow ssh
sudo ufw enable
sudo ufw status verbose
```

> La salida debería ser algo así como:
```output
Status: active
Logging: on (low)
Default: deny (incoming), allow (outgoing), disabled (routed)
New profiles: skip

To                         Action      From
--                         ------      ----
22/tcp                     ALLOW IN    Anywhere                  
22/tcp (v6)                ALLOW IN    Anywhere (v6)
```

4. Asegurarse de que se escuchen los puertos de sshd
```bash
sudo lsof -i -P -n | grep LISTEN
```

> La salida debería ser algo así como:
```output
systemd-r  697 systemd-resolve   14u  IPv4  19989      0t0  TCP 127.0.0.53:53 (LISTEN)
sshd       778            root    3u  IPv4  20570      0t0  TCP *:22 (LISTEN)
sshd       778            root    4u  IPv6  20624      0t0  TCP *:22 (LISTEN)
apache2    783            root    4u  IPv6  20598      0t0  TCP *:80 (LISTEN)
code-e170 1313           utnso    9u  IPv4  22229      0t0  TCP 127.0.0.1:39375 (LISTEN)
apache2   2816        www-data    4u  IPv6  20598      0t0  TCP *:80 (LISTEN)
apache2   2817        www-data    4u  IPv6  20598      0t0  TCP *:80 (LISTEN)
```

5. Editar el archivo de sshd_config
```bash
sudo vi /etc/ssh/sshd_config
```

> Debajo de esta línea:
`Include /etc/ssh/sshd_config.d/*.conf`

> Habilitar al usuario `utnso` para conectarse vía SSH con la VM.
> Cualquier IP será admitida, siempre y cuando ingrese el usuario y contraseña correctos.
```console
AllowUsers utnso
```

6. Reiniciar el servicio de ssh
```bash
sudo systemctl restart ssh
```

Tip. Para revisar el log de SSH:
> Puede ser útil para averigüar la IP del Host: ver qué IPs intentaron conectarse vía SSH con la VM
```bash
less /var/log/auth.log
```

# En el Host (Windows):

-----------------------------

## 23. Conectarse por SSH a la VM

> Conocer la IP del Host
```powershell
ipconfig
```

### Alternativa 1: Usar PowerShell

```powershell
ssh utnso@NúmeroIP -p 22
```
> Nota: el puerto por defecto para SSH es 22

### Alternativa 2: Usar PuTTY

- https://www.putty.org/

[![PuTTY](https://docs.utnso.com.ar/img/guias/herramientas/deploy/deploy-02.jpg)](https://www.putty.org/ "https://www.putty.org/")

- En `Host Name (or IP address)` poner la IP dentro de la VM Guest, Ubuntu Server
- En `Port` poner `22`
- En `Connection type` seleccionar la casilla `SSH`
- Seleccionar el botón `Save`
- En `Close window on exit` seleccionar la casilla `Never`

### Alternativa 3: Usar Cmder

- https://cmder.app/
- https://github.com/cmderdev/cmder/blob/master/README.md

-----------------------------

## 24. (NO en el Deploy) Configurar VSCode

### Extensiones:
	Remote - SSH
	Remote - SSH: Editing Configuration Files
	Remote Explorer
	WSL
	C/C++
	C/C++ Extension Pack
	C/C++ Themes
	GitLens
	Live Share
	Makefile Tools
	Markdown All In One
	Markdown Preview Github Styling
	Notepad++ keymap
	Output Colorizer

> https://code.visualstudio.com/docs/remote/troubleshooting#_installing-a-supported-ssh-client
> https://code.visualstudio.com/docs/remote/troubleshooting#_installing-a-supported-ssh-server

Por defecto, la extensión `Remote - SSH` busca el programa `ssh` utilizando todas nuestras variables de entorno.
En la configuración de la extensión podemos indicarle la ruta hacia el programa `ssh`
> \> Remote-SSH: Settings
```text
Remote.SSH: Path
An absolute path to the SSH executable. When empty, it will use "ssh" on the path or in common install locations.
```

Si hay que instalar `ssh`, una forma es a través de `OpenSSH`: véase el `Anexo 5`.
- "C:\WINDOWS\System32\OpenSSH\ssh.exe -V"
- "C:\Program Files\Git\usr\bin\ssh.exe -V"
- "C:\Program Files (x86)\Git\usr\bin\ssh.exe -V"

Para conectarse por SSH:
1. Presionar el botón azul en la esquina inferior izquierda de VSCode

2. Seleccionar la opción `Connect to Host...`

3. Ingresar en el siguiente formato:
`utnso@NúmeroIPdeVMServer`

4. Guardar la configuración en:
`C:\Users\alumno\.ssh\config`
> En lugar de:
`C:\ProgramData\ssh\ssh_config`
> \> Remote-SSH: Open SSH Configuration File...

5. Revisar el archivo anterior:
```output
# Nombre
Host NúmeroIP
  HostName NúmeroIP
  User utnso
  Port 22
```

# En la VM Guest (Ubuntu Server):

-----------------------------

## 25. Configurar Git en la VM

```bash
git config --global user.email 'ejemplo@frba.utn.edu.ar'
git config --global user.name 'Nombre y Apellido(s)'
```

-----------------------------

## 26. Autenticarse en Git en la VM

# Alternativa 1: Comando gh
1. Instalar `gh` (no viene instalado en la VM de Ubuntu Server)
```bash
sudo apt install gh -y
```

2. Loguearse
```bash
gh auth login
```

# Alternativa 2: Generar claves SSH
## En la VM Server:
```bash
ssh-keygen -t ed25519 -C 'your@email.com'
```
Donde `your@email.com` es el email que tienen asociado a su cuenta de GitHub.

Esto crea una nueva clavev SSH, usando el email provisto como una etiqueta.

> Generating public/private ALGORITHM key pair.
When you're prompted to `Enter a file in which to save the key`,
you can press Enter to accept the default file location.
Please note that if you created SSH keys previously,
ssh-keygen may ask you to rewrite another key,
in which case we recommend creating a custom-named SSH key.
To do so, type the default file location and replace id_ALGORITHM with your custom key name.
```text
> Enter file in which to save the key (/c/Users/YOU/.ssh/id_ALGORITHM):[Press enter]
```

Cuando se te solicite, escribí una frase de contraseña segura. Para más información, mirá: "Working with SSH key passphrases." (https://docs.github.com/en/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account)
```text
> Enter passphrase (empty for no passphrase): [Type a passphrase]
> Enter same passphrase again: [Type passphrase again]
```

Por defecto, se van a guardar en el directorio `~/.ssh/` con los nombres `id_ed25519` y `id_ed25519.pub`

## En el Host Windows:
Por último, vamos a agregar la clave pública a nuestra cuenta de GitHub moviéndonos a `Settings` > `SSH and GPG keys` > `New SSH key`:
- https://github.com/settings/keys
	- https://github.com/settings/ssh/new

1. Completar el campo `Title`
2. En `Key Type`seleccionar `Authentication Key`
3. En `Key` poner la clave pública generado, la cual se puede ver con el comando `cat ~/.ssh/id_ed25519.pub`
4. Seleccionar `Add SSH Key`
5. Si se le solicita, confirma el acceso a tu cuenta de GitHub.

## En la VM Server:

Una vez hecho esto, podemos verificar que todo está configurado correctamente con el comando:
```bash
ssh -T git@github.com
```

La primera vez nos va a preguntar si queremos agregar la clave a la lista de hosts conocidos:
```txt
The authenticity of host 'github.com' can't be established.
ED25519 key fingerprint is SHA256:+asdrfadfasfsdf/asdfsdafsdafdsafdf.
This key is not known by any other names
Are you sure you want to continue connecting (yes/no/[fingerprint])?
```
Vamos a responder `yes` para agregar la clave a la lista de hosts conocidos y poder autenticarnos.

Si todo salió bien, deberíamos ver un mensaje de bienvenida de GitHub:
```txt
Hi TuUsuarioDeGitHub! You've successfully authenticated, but GitHub does not provide shell access.
```

# Alternativa 3: Autenticarse cada vez que se pida
Cada vez que clonemos por ejemplo el repositorio nos pedirá nuestras credenciales:
- `Username for '...'`: Ingresar Nuestro nombre de usuario de GitHub (no nombre que se muestra).
- `Password for '...'`: Ingresar la clave PAT (Personal Access Token) que generaste.

-----------------------------

## 27. Clonar este repositorio (sin --recurse-submodules)

1. Convertir CRLF a LF
```bash
git config --global core.autocrlf input
```

1. Asegurar que todos los archivos del repositorio utilicen LF
```bash
git config --global core.eol lf
```

2. Desactivar la compresión
```bash
git config --global core.compression 0
```

3. Incrementar el tamaño del buffer
Se puede probar con distintos valores: 150MB (157286400), 500MB (524288000)
```bash
git config --global http.postBuffer 524288000
git config --global http.postBuffer 157286400
```

4. Clonar (sin --recurse-submodules)
```bash
cd ~ ; git clone --depth 1 --branch main --single-branch --no-tags https://github.com/sisoputnfrba/tp-2024-2c-so
```

> Debería ser equivalente a:
```bash
cd /home/utnso ; git clone --depth 1 --branch main --single-branch --no-tags https://github.com/sisoputnfrba/tp-2024-2c-so
```

5. Moverse a la rama que corresponda
```bash
git checkout main
```

-----------------------------

## 28. Clonarse los submódulos del repositorio
```bash
cd tp-2024-2c-so ; git submodule update --init --recursive --depth 1
```

-----------------------------

## 29. Instalar la SO Commons Library

```bash
cd /home/utnso/tp-2024-2c-so/thirdparty/so-commons-library
make debug
sudo make install
```

Por si el `make install` falla:
```bash
sudo cp build/libcommons.so /usr/lib
sudo cp src/commons /usr/include
```

-----------------------------

## 30. (NO en el Deploy) Instalar CSpec

```bash
cd /home/utnso/tp-2024-2c-so/thirdparty/cspec
make debug
sudo make install
```

Por si el `make install` falla:
```bash
sudo cp release/libcspecs.so /usr/lib
sudo cp src/cspecs /usr/include
```

-----------------------------

## 31. (NO en el Deploy) Instalar la versión más reciente de CMake

- https://cmake.org/download/
	- https://github.com/Kitware/CMake/releases/download/v3.29.0/cmake-3.29.0-linux-x86_64.sh

```bash
wget https://github.com/Kitware/CMake/releases/download/v3.29.0/cmake-3.29.0-linux-x86_64.sh
chmod +x cmake-3.29.0-linux-x86_64.sh
sudo ./cmake-3.29.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

-----------------------------

## 32. Crear un acceso directo (soft link / symlink) a los archivos de test

Deben quedar en: `/home/utnso/scripts-pruebas`/...
```bash
ln -s /home/utnso/tp-2024-2c-so/thirdparty/the-last-of-c-pruebas /home/utnso/scripts-pruebas
```

-----------------------------

## 33. Poner las IPs (*los Puertos no*) en los archivos de config

Completar con las IPs que te hayan pasado tus compañeros de equipo que correspondan a las máquinas
corriendo los módulos
```bash
bash scripts/ip.sh
```

-----------------------------

## 34. Copiar las configs de una prueba

Inicialmente pondremos la de la primera prueba
```bash
clear ; bash scripts/cp_configs.sh ; bash scripts/kernel_exec.sh
```

-----------------------------

## 35. (Entre prueba y prueba) Borrar los archivos de log y los creados por Filesystem

```bash
bash scripts/log_rm.sh ; bash scripts/fs_rm.sh
```

-----------------------------

## 36. Levantar los módulo(s) del TP que correspondan

Los comandos del makefile están en el `Anexo 5`

-----------------------------


## Anexo 1: Snapshots (Instantáneas) de la VM

https://youtu.be/u1L23ziKgz4

-----------------------------

## Anexo 2: Guardar el estado de una VM

https://youtu.be/YqFybzQmqOc

-----------------------------

## Anexo 3: Clonar la VM

Para simular que contamos con dos máquinas distintas, lo que haremos será clonar la VM que ya tenemos configurada.
Para ello, vamos a hacer click derecho en la VM > "Clone..." y seguir los pasos para clonar la VM.
Recomendamos usar la opción "Linked clone" ya que el proceso de clonado es más rápido y ocupa menos espacio:

[![vm-clone](https://docs.utnso.com.ar/img/primeros-pasos/tp0/vm-clone.png)]()

### Resolver conflictos de red
Por último, vamos a iniciar sesión en una de las VMs y ejecutar las siguientes 3 líneas:

```bash
sudo rm -f /etc/machine-id
sudo dbus-uuidgen --ensure=/etc/machine-id
sudo reboot
```

Esto lo que hace es generar un nuevo archivo `/etc/machine-id`,
en el cual se guarda un identificador que permite al router
asignarle la misma IP a una máquina cada vez que éstase conecta a la red.

Al nosotros haber clonado la misma VM, ambas tienen el mismo `machine-id`,
por lo que el router podría terminar asignándoles la misma IP a ambas VMs,
lo cual generaría conflictos a la hora de conectarlas en red.

Luego de reiniciar, ejecuten `ifconfig` para corroborar que efectivamente las IPs de todas las VMs son distintas.

-----------------------------

## Anexo 4: En caso de que falle al clonar y/o tarde mucho

Suele ser por el peso y el poco ancho de banda disponible en el momento (por estar la red saturada por ejemplo)

- Alternativa 1: Resumir el clonado fallido
```bash
cd tp-2024-2c-so
git fetch --all
```

- Alternativa 2: Clonar utilizando SSH en lugar de HTTPS:
```bash
git clone git@github.com:sisoputnfrba/tp-2024-2c-so.git
```

- Alternativa 3: Descargar el repositorio:

Fuente: https://docs.github.com/es/rest/repos/contents?apiVersion=2022-11-28#get-archive-link

### Descargar un repositorio como un archivo ZIP
- cURL
```bash
curl -L \
  -H "Accept: application/vnd.github+json" \
  -H "Authorization: Bearer TU_TOKEN_DE_GITHUB" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  https://api.github.com/repos/sisoputnfrba/tp-2024-2c-so/zipball/main \
  -o 'tp-2024-2c-so-main.zip'
```

- CLI de GitHub
```bash
gh api \
  -H "Accept: application/vnd.github+json" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  /repos/sisoputnfrba/tp-2024-2c-so/zipball/main
```

### Descargar un repositorio como un archivo TAR
- cURL
```bash
curl -L \
  -H "Accept: application/vnd.github+json" \
  -H "Authorization: Bearer TU_TOKEN_DE_GITHUB" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  https://api.github.com/repos/sisoputnfrba/tp-2024-2c-so/tarball/main \
  -o 'tp-2024-2c-so-main.tar.gz'
```

- CLI de GitHub
```bash
gh api \
  -H "Accept: application/vnd.github+json" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  /repos/sisoputnfrba/tp-2024-2c-so/tarball/main
```

-----------------------------

## Anexo 5: Comandos generales

- Apagar
```bash
shutdown now
```

> Alternativa:
```bash
init 0
```

- Reiniciar
```bash
shutdown -r now
```

> Alternativa 1:
```bash
reboot
```

> Alternativa 2:
```bash
init 6
```

- Activar impresión de comandos ejecutados
```bash
set -x
```

- Desactivar impresión de comandos ejecutados
```bash
{ set +x ; } 2>/dev/null
```

- Cambiar a root
```bash
sudo -s
```

> Alternativa:
```bash
sudo su
```

- Salir de root
```bash
exit
```

- Listar las opciones de montaje:
```bash
mount
```

> Listar las Carpetas Compartidas de Virtual Box (vboxsf)
```bash
mount | grep vboxsf
```

- Desmontar un disco
```bash
sudo umount /ruta/al/punto/de/montaje
```

- Listar configuración de red
```bash
ifconfig
```

- Imprimir directorio actual
```bash
pwd
```

- Cambiar de directorio
```bash
cd <ruta>
```

> Home (*/home/utnso*)
```bash
cd ~
```

> Directorio padre
```bash
cd ..
```

> Directorio anterior
```bash
cd -
```

- Listar archivos (excepto ocultos) (con formato)
```bash
ls
```

- Listar archivos (incluido ocultos) (con formato)
```bash
ls -a
```

- Listar permisos de archivos
```bash
ls -l
```

- Listar archivos (sin formato)
```bash
dir
```

- Imprimir árbol de archivos y directorios
```bash
tree
```

- Buscar archivo(s) y/o directorio(s) en todos los subdirectorios
```bash
find .
```

> Buscar archivos de log en todos los subdirectorios
```bash
find . -type f \( -name '*.log' \) -print
```

- Cambiar owner al usuario actual
```bash
find . -type f ! -path "./.git/*" -exec chown $USER {} +
```

- Listar CRLF o LF por cada archivo
```bash
find . -type f -exec bash -c 'file="{}"; if file "$file" | grep -q "CRLF"; then echo "$file CRLF"; else echo "$file LF"; fi' \;
```

- Cambiar de CRLF a LF en todos los archivos \*.sh y \*.config (excluyendo los de .git)
```bash
sudo apt install -y dos2unix
find . -type f \( -name "*.sh" -o -name "*.config" \) ! -path "./.git/*" -exec dos2unix {} +
```

- Cambiar de LF a CRLF en todos los archivos \*.sh y \*.config (excluyendo los de .git)
```bash
sudo apt install -y unix2dos
unix2dos <archivo>
```

- Crear directorio vacío
```bash
mkdir <directorio>
```

- Crear archivo vacío
```bash
touch <archivo>
```

- Copiar archivo(s) y/o directorio(s) de origen a destino
```bash
cp <origen> <destino>
```

- Mover archivo(s) y/o directorio(s) de origen a destino
```bash
mv <origen> <destino>
```

> Mover todos los archivos (incluidos los ocultos)
```bash
mv -f /path/subdirectory/{.,}* /path/
```

> Este comando se expande a:
```bash
mv /path/subdirectory/* /path/subdirectory/.* /path/
```
> El asterisco (*) representa todos los archivos en la carpeta subdirectorio, y el punto asterisco (.*) representa a todos los archivos ocultos en la carpeta subdirectorio. Ambos tipos de archivos serán movidos al directorio destino.

- Eliminar archivo(s) y/o directorio(s) recursivamente y forzosamente
```bash
rm -rf <directorio>
```

- Eliminar directorio si solo si esta vacio, no está en uso, etc.
```bash
rmdir <directorio>
```

- Leer un archivo de texto
```bash
less <archivo>
```

	- https://www.redswitches.com/blog/less-command-in-linux/#:~:text=What%20is%20the%20less%20command,the%20file%20and%20forward%20navigation.

- Monitorear un archivo de texto en tiempo real
```bash
less +F <archivo>
```

	- Con Ctrl + C podemos pausar el monitoreo para scrollear por el archivo usando las flechas
	- Con Shift + F podemos continuar con el seguimiento que habíamos pausado.

- Imprimir el contenido de un archivo en hexadecimal
```bash
hexdump -C bloques.dat
```

- Imprimir el contenido de un archivo en hexadecimal
```bash
xxd bloques.dat
```

- Imprimir el contenido de un archivo en binario
```bash
xxd -b bitmap.dat
```

- Editar un archivo de texto
	- Con `vi`
```bash
vi <archivo>
```

	- Con `vim`
```bash
vim <archivo>
```

	- Con `nano`
```bash
nano <archivo>
```

- Combinar archivos de log
```bash
cat src/cpu/cpu.log src/cpu/minimal.log | sort -k2,2 > combinado.log
```

		- En la última línea pueden ver las distintas opciones que se pueden usar.
			- Por ejemplo: Ctrl + X para salir.

- Administrador de procesos
```bash
htop
```

[![htop](https://docs.utnso.com.ar/img/guias/consola/bash-htop-espera-activa.png)]()

- Sirve entre otras cosas para:
	- Visualizar el uso de CPU y RAM (para detectar esperas activas y memory leaks).
	- Ordenar los procesos por PID, nombre, uso de CPU/RAM, etc. con el mouse.
	- Filtrar los procesos (e hilos KLT) por nombre con F4.
	- Enviar señales a uno o varios procesos de forma intuitiva con F9.

- Listar todos los archivos abiertos
```bash
lsof
```

> En las entregas, puede ser muy útil junto con el flag -i para corroborar que no haya ningún proceso escuchando en un puerto en particular. Ejemplo:
```bash
lsof -i :8080
```

- Terminar un proceso forzosamente
```bash
kill -s SIGKILL <PID>
```

- Listar todas las variables de entorno:
```bash
env
```

> Encontrar variable más fácilmente
```bash
env | grep NOMBRE_VARIABLE
```

> Imprimir valor de variable
```bash
echo $NOMBRE_VARIABLE
```

- Configurar una variable de entorno para la sesión actual:
```bash
export NOMBRE_VARIABLE='un valor'
```

> Para definir una variable que valga para todas las sesiones, podemos hacerlo agregando el export al final del archivo `~/.bashrc`

- Cambiar el layout del teclado
> A Inglés (Estados Unidos)
```bash
sudo loadkeys us
```
> A Español (Argentina)
```bash
sudo loadkeys es
```

- Cambiar el dueño de un archivo (cambiar propietario)
```bash
chmod
```

> Para cambiar el ownership de un archivo a mi usuario
```bash
chown $USER ejemplo.txt
```

> Para cambiar el ownership de una carpeta y todo su contenido
```bash
chown -R $USER /home/utnso/swap
```

- Cambiar los permisos de un archivo (cambiar de modo)
```bash
chmod
```

> Para dar permisos de ejecución
```bash
chmod +x mi-script.sh
```

> Para configurar nuevos permisos usando el formato Unix
```bash
chmod 777 kernel.config
```

> Machete para escribir los permisos tipo Unix en octal:
[![chmod](https://docs.utnso.com.ar/img/guias/consola/bash-linux-file-permissions.jpg)]()

- Listar todos los usuarios:
```bash
users
```

- Listar todos los grupos:
```bash
groups
```

- Listar todos los grupos a los que pertenece un usuario:
```bash
groups utnso
groups root
```

- Listar ids
```bash
id
id -u
id -g
```

-----------------------------

## Anexo 6: Comandos con el makefile

- Compilar todos los módulos
```bash
clear ; make -j -O
```

> Es equivalente a:
```bash
clear ; make all -j -O
```

- Compilar un módulo en particular con las utils
```bash
clear ; make src/kernel/bin/kernel -j -O
clear ; make src/cpu/bin/cpu -j -O
clear ; make src/memoria/bin/memoria -j -O
clear ; make src/filesystem/bin/filesystem -j -O
```

- Compilar sólo las utils
```bash
clear ; make src/utils/bin/libutils.a -j -O
```

- Borrar todo lo que se genera al compilar
```bash
clear ; make cleandirs
```

- Ejecutar un módulo
```bash
clear ; make run-kernel 'kernel_ARGS=PLANI_PROC 32'
clear ; make run-cpu 'cpu_ARGS='
clear ; make run-memoria 'memoria_ARGS='
clear ; make run-filesystem 'filesystem_ARGS='
```

- Ejecutar con memcheck un módulo
```bash
clear ; make valgrind-memcheck-kernel 'kernel_ARGS=PLANI_PROC 32'
clear ; make valgrind-memcheck-cpu 'cpu_ARGS='
clear ; make valgrind-memcheck-memoria 'memoria_ARGS='
clear ; make valgrind-memcheck-filesystem 'filesystem_ARGS='
```

- Ejecutar con helgrind un módulo
```bash
clear ; make valgrind-helgrind-kernel 'kernel_ARGS='
clear ; make valgrind-helgrind-cpu 'cpu_ARGS='
clear ; make valgrind-helgrind-memoria 'memoria_ARGS='
clear ; make valgrind-helgrind-filesystem 'filesystem_ARGS='
```

- Ejecutar con valgrind (sin ninguna herramienta) un módulo
```bash
clear ; make valgrind-none-kernel 'kernel_ARGS='
clear ; make valgrind-none-cpu 'cpu_ARGS='
clear ; make valgrind-none-memoria 'memoria_ARGS='
clear ; make valgrind-none-filesystem 'filesystem_ARGS='
```

-----------------------------

## Anexo 7: Opciones importantes de gcc

`-DDEBUG -fdiagnostics-color=always -lcommons -lpthread -lreadline -lm`

-----------------------------

## Anexo 8: Comandos de Git

- Variables interesantes
```bash
core.autocrlf
core.eol
core.safecrlf
```

- Settear una variable
```bash
git config --global <VARIABLE> <VALOR>
```

- Unsettear una variable
```bash
git config --global --unset <VARIABLE>
```

- Obtener el valor de una variable
```bash
git config --global --get <VARIABLE>
```

- Moverse a un commit específico de un repositorio
```bash
git checkout -q <commit-hash>
```

- Reclonar un repositorio
```bash
git rm --cached -r .
git reset --hard
```

- Clonar un repositorio con todos sus submódulos
```bash
git clone --recurse-submodules
```

- Clonar todos los submódulos de un repositorio ya clonado
```bash
git submodule update --init --recursive
```

- Agregar un submodulo
```bash
git submodule add https://github.com/sisoputnfrba/the-last-of-c-pruebas thirdparty/the-last-of-c-pruebas
```

- Saber a qué commit hash apunta el submódulo
```bash
cd submodules/mysubmodule
git rev-parse HEAD
```

- Actualizar todos los submódulos hasta sus últimos commits
```bash
git submodule update --remote --merge
```

- Actualizar manualmente el commit hash al que apunta un submódulo
```bash
cd submodules/mysubmodule
git fetch
git pull

Opcional:
git checkout -q <commit-hash>

cd -
git add submodules/mysubmodule
git commit -m "Update submodule to new commit"
```

- Eliminar un submódulo
```bash
git submodule deinit <submodule>
git rm <submodule>
```

- Merge por defecto al pullear
```bash
git config --global pull.rebase false
```

- Rebase por defecto al pullear
```bash
git config --global pull.rebase true
```

- Fast-forward (ff) por defecto al pullear
```bash
git config --global pull.ff only
```

- Merge override de la configuración por defecto en un pull en específico
```bash
git pull --no-rebase
```

- Rebase override de la configuración por defecto en un pull en específico
```bash
git pull --rebase
```

- Fast-forward (ff) override de la configuración por defecto en un pull en específico
```bash
git pull --ff-only
```

-----------------------------

## Anexo 9: Duración de tests

> 46s
- configs/1.1-Prueba_Planificacion_FIFO/

> 48s
- configs/1.2-Prueba_Planificacion_PRIORIDADES/

> 1m39.353s
- configs/1.3-Prueba_Planificacion_CMN/

> 4m58.138s
- configs/2.1-Prueba_Race_Condition_750_QUANTUM/

> 7m23.576s
- configs/2.1-Prueba_Race_Condition_150_QUANTUM/

> 2m05.561s + 8 * duración de las IO (2 minutos por default c/u)
> 2m05.561s - 108s
- configs/3.1-Prueba_Particiones_Fijas_FIRST_ALGORITMO_BUSQUEDA/

- configs/3.2-Prueba_Particiones_Fijas_BEST_ALGORITMO_BUSQUEDA/

- configs/3.3-Prueba_Particiones_Fijas_WORST_ALGORITMO_BUSQUEDA/

> LOOP INFINITO (después de 5 minutos se repiten los mismos hilos)
- configs/4-Prueba_Particiones_Dinamicas/

> Primera vuelta: 5m50.574s
> Segunda vuelta: 5m50.574s
- configs/5-Prueba_FS_Fibonacci_Sequence/
> Bloque 178: Primera vuelta
> Bloque 196: Segunda vuelta último bloque escrito antes de no poder escribir más

> LOOP INFINITO (después de n minutos)
- configs/6-Prueba_de_Stress/

-----------------------------

Prueba CPU Race Condition

```bash
cat src/cpu/cpu.log | grep -i "LOG DX:"
```

150
[INFO] 00:46:45:204 CPU/(246759:246759): (1:1) LOG DX: 1
[INFO] 00:46:45:854 CPU/(246759:246759): (1:2) LOG DX: 1
[INFO] 00:46:46:505 CPU/(246759:246759): (1:3) LOG DX: 1
[INFO] 00:46:47:163 CPU/(246759:246759): (1:4) LOG DX: 1
[INFO] 00:47:02:677 CPU/(246759:246759): (1:1) LOG DX: 2
[INFO] 00:47:03:326 CPU/(246759:246759): (1:2) LOG DX: 2
[INFO] 00:47:03:979 CPU/(246759:246759): (1:3) LOG DX: 2
[INFO] 00:47:04:630 CPU/(246759:246759): (1:4) LOG DX: 2
[INFO] 00:47:20:229 CPU/(246759:246759): (1:1) LOG DX: 3
[INFO] 00:47:20:884 CPU/(246759:246759): (1:2) LOG DX: 3
[INFO] 00:47:21:550 CPU/(246759:246759): (1:3) LOG DX: 3
[INFO] 00:47:22:212 CPU/(246759:246759): (1:4) LOG DX: 3
[INFO] 00:47:37:735 CPU/(246759:246759): (1:1) LOG DX: 4
[INFO] 00:47:38:418 CPU/(246759:246759): (1:2) LOG DX: 4
[INFO] 00:47:39:071 CPU/(246759:246759): (1:3) LOG DX: 4
[INFO] 00:47:39:727 CPU/(246759:246759): (1:4) LOG DX: 4
[INFO] 00:47:55:355 CPU/(246759:246759): (1:1) LOG DX: 5
[INFO] 00:47:56:104 CPU/(246759:246759): (1:2) LOG DX: 5
[INFO] 00:47:56:791 CPU/(246759:246759): (1:3) LOG DX: 5
[INFO] 00:47:57:441 CPU/(246759:246759): (1:4) LOG DX: 5
[INFO] 00:48:13:085 CPU/(246759:246759): (1:1) LOG DX: 6
[INFO] 00:48:13:738 CPU/(246759:246759): (1:2) LOG DX: 6
[INFO] 00:48:14:395 CPU/(246759:246759): (1:3) LOG DX: 6
[INFO] 00:48:15:070 CPU/(246759:246759): (1:4) LOG DX: 6
[INFO] 00:48:30:617 CPU/(246759:246759): (1:1) LOG DX: 7
[INFO] 00:48:31:273 CPU/(246759:246759): (1:2) LOG DX: 7
[INFO] 00:48:31:950 CPU/(246759:246759): (1:3) LOG DX: 7
[INFO] 00:48:32:614 CPU/(246759:246759): (1:4) LOG DX: 7
[INFO] 00:48:48:274 CPU/(246759:246759): (1:1) LOG DX: 8
[INFO] 00:48:48:926 CPU/(246759:246759): (1:2) LOG DX: 8
[INFO] 00:48:49:612 CPU/(246759:246759): (1:3) LOG DX: 8
[INFO] 00:48:50:263 CPU/(246759:246759): (1:4) LOG DX: 8
[INFO] 00:49:06:019 CPU/(246759:246759): (1:1) LOG DX: 9
[INFO] 00:49:06:673 CPU/(246759:246759): (1:2) LOG DX: 9
[INFO] 00:49:07:335 CPU/(246759:246759): (1:3) LOG DX: 9
[INFO] 00:49:07:985 CPU/(246759:246759): (1:4) LOG DX: 9
[INFO] 00:49:34:199 CPU/(246759:246759): (1:1) LOG DX: 10
[INFO] 00:49:34:854 CPU/(246759:246759): (1:2) LOG DX: 10
[INFO] 00:49:35:511 CPU/(246759:246759): (1:3) LOG DX: 10
[INFO] 00:49:36:175 CPU/(246759:246759): (1:4) LOG DX: 10
[INFO] 00:49:44:903 CPU/(246759:246759): (1:0) LOG DX: 10

-----------------------------

750
[INFO] 00:24:46:429 CPU/(240094:240094): (1:1) LOG DX: 1
[INFO] 00:24:47:500 CPU/(240094:240094): (1:2) LOG DX: 1
[INFO] 00:24:48:582 CPU/(240094:240094): (1:3) LOG DX: 1
[INFO] 00:24:49:648 CPU/(240094:240094): (1:4) LOG DX: 1
[INFO] 00:27:51:465 CPU/(241774:241774): (1:1) LOG DX: 1
[INFO] 00:27:52:533 CPU/(241774:241774): (1:2) LOG DX: 1
[INFO] 00:27:53:608 CPU/(241774:241774): (1:3) LOG DX: 1
[INFO] 00:27:54:676 CPU/(241774:241774): (1:4) LOG DX: 1
[INFO] 00:28:00:460 CPU/(241774:241774): (1:1) LOG DX: 2
[INFO] 00:28:01:521 CPU/(241774:241774): (1:2) LOG DX: 2
[INFO] 00:28:02:580 CPU/(241774:241774): (1:3) LOG DX: 2
[INFO] 00:28:03:652 CPU/(241774:241774): (1:4) LOG DX: 2
[INFO] 00:28:13:799 CPU/(241774:241774): (1:1) LOG DX: 3
[INFO] 00:28:14:870 CPU/(241774:241774): (1:2) LOG DX: 3
[INFO] 00:28:15:926 CPU/(241774:241774): (1:3) LOG DX: 3
[INFO] 00:28:16:989 CPU/(241774:241774): (1:4) LOG DX: 3
[INFO] 00:28:22:731 CPU/(241774:241774): (1:1) LOG DX: 4
[INFO] 00:28:23:844 CPU/(241774:241774): (1:2) LOG DX: 4
[INFO] 00:28:24:907 CPU/(241774:241774): (1:3) LOG DX: 4
[INFO] 00:28:25:995 CPU/(241774:241774): (1:4) LOG DX: 4
[INFO] 00:28:36:084 CPU/(241774:241774): (1:1) LOG DX: 5
[INFO] 00:28:37:153 CPU/(241774:241774): (1:2) LOG DX: 5
[INFO] 00:28:38:217 CPU/(241774:241774): (1:3) LOG DX: 5
[INFO] 00:28:39:287 CPU/(241774:241774): (1:4) LOG DX: 5
[INFO] 00:28:45:129 CPU/(241774:241774): (1:1) LOG DX: 6
[INFO] 00:28:46:191 CPU/(241774:241774): (1:2) LOG DX: 6
[INFO] 00:28:47:265 CPU/(241774:241774): (1:3) LOG DX: 6
[INFO] 00:28:48:340 CPU/(241774:241774): (1:4) LOG DX: 6
[INFO] 00:28:58:346 CPU/(241774:241774): (1:1) LOG DX: 7
[INFO] 00:28:59:406 CPU/(241774:241774): (1:2) LOG DX: 7
[INFO] 00:29:00:464 CPU/(241774:241774): (1:3) LOG DX: 7
[INFO] 00:29:01:551 CPU/(241774:241774): (1:4) LOG DX: 7
[INFO] 00:29:07:329 CPU/(241774:241774): (1:1) LOG DX: 8
[INFO] 00:29:08:404 CPU/(241774:241774): (1:2) LOG DX: 8
[INFO] 00:29:09:484 CPU/(241774:241774): (1:3) LOG DX: 8
[INFO] 00:29:10:557 CPU/(241774:241774): (1:4) LOG DX: 8
[INFO] 00:29:20:625 CPU/(241774:241774): (1:1) LOG DX: 9
[INFO] 00:29:21:688 CPU/(241774:241774): (1:2) LOG DX: 9
[INFO] 00:29:22:744 CPU/(241774:241774): (1:3) LOG DX: 9
[INFO] 00:29:23:819 CPU/(241774:241774): (1:4) LOG DX: 9
[INFO] 00:29:29:579 CPU/(241774:241774): (1:1) LOG DX: 10
[INFO] 00:29:30:648 CPU/(241774:241774): (1:2) LOG DX: 10
[INFO] 00:29:31:721 CPU/(241774:241774): (1:3) LOG DX: 10
[INFO] 00:29:32:787 CPU/(241774:241774): (1:4) LOG DX: 10
[INFO] 00:29:39:521 CPU/(241774:241774): (1:0) LOG DX: 10

-----------------------------

## Anexo 10: tmux (Terminal MUltipleXer)

Referencia: https://gist.github.com/MohamedAlaa/2961058

- Abrir una sesión nueva
```bash
tmux
```

> Equivalente:
```bash
tmux new
```

- Abrir una sesión nueva y ponerle un nombre:
```bash
tmux new -s 'NombreDeSesion'
```

- Cerrar la ventana actual de una sesión
```bash
exit
```

- Para cerrar forzosamente la ventana de la sesion, presione <kbd>Ctrl</kbd> + <kbd>b</kbd>, seguidamente presione <kbd>x</kbd> y por ultimo presione <kbd>y</kbd>
- Para crear una nueva ventana, presione <kbd>Ctrl</kbd> + <kbd>b</kbd> y seguidamente presione <kbd>c</kbd>
- Para apartar la sesion con sus ventanas sin cerrarla [detach], presione <kbd>Ctrl</kbd> + <kbd>b</kbd> y seguidamente presione <kbd>d</kbd>
- Para volver a las sesiones apartadas de `tmux [detached]`, ejecute el comando:
```bash
tmux attach
```

> Por el número identificador de la sesión
```bash
tmux attach -t 0
```

> Por el nombre de la sesión
```bash
tmux attach -t 'NombreDeSesion'
```
- Para listar cuántas sesiones de tmux están abiertas, ejecute el comando:
```bash
tmux ls
```
- Para cerrar forzosamente una sesión de tmux, ejecute el comando:
```bash
tmux kill-session
```

> Por el número identificador de la sesión
```bash
tmux kill -t 0
```

> Por el nombre de la sesión
```bash
tmux kill -t 'NombreDeSesion'
```

- Para cambiarle el nombre a la sesion actual de tmux, presione <kbd>Ctrl</kbd> + <kbd>b</kbd> y seguidamente presione <kbd>,</kbd>
- Para alternar entre las sesiones abiertas de tmux, presione <kbd>Ctrl</kbd> + <kbd>b</kbd> y seguidamente presione <kbd>s</kbd>
- Para alternar entre las ventanas de las sesiones abiertas de tmux, presione <kbd>Ctrl</kbd> + <kbd>b</kbd> y seguidamente presione <kbd>w</kbd>
- Para iniciar el modo desplazamiento por la ventana, presione <kbd>Ctrl</kbd> + <kbd>b</kbd> y seguidamente presione <kbd>[</kbd> (con la distribucion de teclado latinoamericano, <kbd>[</kbd> es <kbd>⇧ Shift</kbd> + <kbd>{</kbd>) ]
- Para finalizar el modo desplazamiento por la ventana, presione <kbd>q</kbd>

-----------------------------

## Anexo 11: Instalar OpenSSH

### Alternativa 1: Con Git para Windows

https://git-scm.com/download/win

Por defecto, OpenSSH ya viene instalado con Git para Windows, en alguna de las siguientes rutas:
- "C:\Program Files\Git\usr\bin\ssh.exe -V"
- "C:\Program Files (x86)\Git\usr\bin\ssh.exe -V"

### Alternativa 2: Desde el repositorio de GitHub

https://github.com/PowerShell/Win32-OpenSSH/releases

### Alternativa 3: Desde PowerShell (Requiere Permisos de Administrador)

> https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_install_firstuse?tabs=powershell#tabpanel_1_powershell

1. To install OpenSSH using PowerShell, run PowerShell as an Administrator. To make sure that OpenSSH is available, run the following cmdlet:

```powershell
Get-WindowsCapability -Online | Where-Object Name -like 'OpenSSH*'
```
The command should return the following output if neither are already installed:

```output
Name  : OpenSSH.Client~~~~0.0.1.0
State : NotPresent

Name  : OpenSSH.Server~~~~0.0.1.0
State : NotPresent
```

2. Then, install the server or client components as needed:

```powershell
# Install the OpenSSH Client
Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0

# Install the OpenSSH Server
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
```
Both commands should return the following output:

```output
Path          :
Online        : True
RestartNeeded : False
```
3. To start and configure OpenSSH Server for initial use, open an elevated PowerShell prompt (right click, Run as an administrator), then run the following commands to start the sshd service:

```powershell
# Start the sshd service
Start-Service sshd

# OPTIONAL but recommended:
Set-Service -Name sshd -StartupType 'Automatic'

# Confirm the Firewall rule is configured. It should be created automatically by setup. Run the following to verify
if (!(Get-NetFirewallRule -Name "OpenSSH-Server-In-TCP" -ErrorAction SilentlyContinue | Select-Object Name, Enabled)) {
    Write-Output "Firewall Rule 'OpenSSH-Server-In-TCP' does not exist, creating it..."
    New-NetFirewallRule -Name 'OpenSSH-Server-In-TCP' -DisplayName 'OpenSSH Server (sshd)' -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22
} else {
    Write-Output "Firewall rule 'OpenSSH-Server-In-TCP' has been created and exists."
}
```

- https://code.visualstudio.com/docs/remote/ssh-tutorial

-----------------------------

## Anexo 12: Duplicar un repositorio de GitHub

1. Crear el nuevo repositorio copia pero sin inicializarlo con un `README`, `.gitignore`, `LICENSE`, ni ningún otro archivo

2. Clonar el repositorio original completo
```bash
git clone --mirror https://github.com/sisoputnfrba/tp-2024-2c-so
```

3.
```bash
cd tp-2024-2c-so.git/
```

4. Configurar la URL del repositorio remoto del repositorio local original como el del copia
```bash
git remote set-url origin https://github.com/USUARIO_GITHUB/NOMBRE_REPOSITORIO_COPIA.git
```

5. Realizar el push "espejo"
```bash
git push --mirror
```

-----------------------------

# Al salir

### 1. Apagar la VM Ubuntu Server

### 2. En VirtualBox, restaurar el snapshot de la VM Ubuntu Server al Base

### 3. Desloguearse de las cuentas del navegador

### 4. Borrar el historial del navegador

### 5. Desloguearse del Git de Windows

### 6. Quitar las credenciales de Git en el Administrador de Credenciales de Windows

### 7. Eliminar el repositorio clonado en Windows

### 8. Vaciar la papelera de reciclaje
