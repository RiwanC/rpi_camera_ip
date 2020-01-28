# Image et compilation

+ Compilation de l'OS

La compilation de l'OS est réalisée à l'aide de BuildRoot.
L'image docker fournie comprend un OS pré-compilé et utilisable en l'état.

Si l'on veut recompiler l'OS, il faut faire un make menuconfig (respectivement make menuconfig busybox pour busybox) pour changer ce qu'on inclut dans l'OS.

Pour que la caméra puisse fonctionner, en cas de recompilation, il ne faut pas oublier d'activer le support de la caméra en activant les options _BR2_PACKAGE_RPI_FIRMWARE_X_ et _BR2_PACKAGE_LIBV4L_.

+ Flashage de l'image

Il faut récupérer l'image résultante puis sur une carte SD disponible à travers /dev/sdX (remplacer X par le chemin vers la carte SD) exécuter la commande suivante :

~~~shell script
$ sudo dd if=sdcard.img of=/dev/sdX
~~~