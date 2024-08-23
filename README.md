# Trabajo Práctico de Sistemas Operativos - 2do Cuatrimestre 2024

## Grupo "so"

| Legajo    | Apellido(s) | Nombre(s)         | Usuario de GitHub                                                  | Correo electrónico institucional  | Curso |
| --------- | ----------- | ----------------- | ------------------------------------------------------------------ | --------------------------------- | ----- |
| 173.065-4 | Maqueda     | Fernando Daniel   | [@fernandodanielmaqueda](https://github.com/fernandodanielmaqueda) | fmaqueda@frba.utn.edu.ar          | K3574 |
| 144.922-9 | Fink        | Brian             | [@Brai93](https://github.com/Brai93)                               | brianfink@frba.utn.edu.ar         | K3774 |
| 149.201-9 | Baldiviezo  | Ramon Angel Vega  | [@rvegabaldiviezo](https://github.com/rvegabaldiviezo)             | rvegabaldiviezo@frba.utn.edu.ar   | K3574 |
| 175.400-2 | Morosini    | Pablo Ariel       | [@MorosiniP](https://github.com/MorosiniP)                         | pmorosini@frba.utn.edu.ar         | K3723 |
| 204.155-8 | Vigilante   | Santiago Fabrizio | [@svigilante](https://github.com/svigilante)                       | svigilante@frba.utn.edu.ar        | K3673 |

## Dependencias

Para poder compilar y ejecutar el proyecto, es necesario tener instalada la
biblioteca [so-commons-library] de la cátedra:

```bash
git clone https://github.com/sisoputnfrba/so-commons-library
cd so-commons-library
make debug
make install
```

## Compilación y ejecución

Cada módulo del proyecto se compila de forma independiente a través de un
archivo `makefile`. Para compilar un módulo, es necesario ejecutar el comando
`make` desde la carpeta correspondiente.

El ejecutable resultante de la compilación se guardará en la carpeta `bin` del
módulo. Ejemplo:

```sh
cd kernel
make
./bin/kernel
```

## Importar desde Visual Studio Code

Para importar el workspace, debemos abrir el archivo `tp.code-workspace` desde
la interfaz o ejecutando el siguiente comando desde la carpeta raíz del
repositorio:

```bash
code tp.code-workspace
```

## Checkpoint

Para cada checkpoint de control obligatorio, se debe crear un tag en el
repositorio con el siguiente formato:

```
checkpoint-{número}
```

Donde `{número}` es el número del checkpoint, ejemplo: `checkpoint-1`.

Para crear un tag y subirlo al repositorio, podemos utilizar los siguientes
comandos:

```bash
git tag -a checkpoint-{número} -m "Checkpoint {número}"
git push origin checkpoint-{número}
```

> [!WARNING]
> Asegúrense de que el código compila y cumple con los requisitos del checkpoint
> antes de subir el tag.

## Entrega

Para desplegar el proyecto en una máquina Ubuntu Server, podemos utilizar el
script [so-deploy] de la cátedra:

```bash
git clone https://github.com/sisoputnfrba/so-deploy.git
cd so-deploy
./deploy.sh -r=release -p=utils -p=kernel -p=cpu -p=memoria -p=filesystem "tp-{año}-{cuatri}-{grupo}"
```

El mismo se encargará de instalar las Commons, clonar el repositorio del grupo
y compilar el proyecto en la máquina remota.

> [!NOTE]
> Ante cualquier duda, pueden consultar la documentación en el repositorio de
> [so-deploy], o utilizar el comando `./deploy.sh --help`.

## Guías útiles

- [Cómo interpretar errores de compilación](https://docs.utnso.com.ar/primeros-pasos/primer-proyecto-c#errores-de-compilacion)
- [Cómo utilizar el debugger](https://docs.utnso.com.ar/guias/herramientas/debugger)
- [Cómo configuramos Visual Studio Code](https://docs.utnso.com.ar/guias/herramientas/code)
- **[Guía de despliegue de TP](https://docs.utnso.com.ar/guías/herramientas/deploy)**

[so-commons-library]: https://github.com/sisoputnfrba/so-commons-library
[so-deploy]: https://github.com/sisoputnfrba/so-deploy
