## **Tutoriel CMake pour la cross-compilation pour rpi sous docker**

- Récupération de l'image Docker:
~~~
$ docker pull pblottiere/embsys-rpi3-buildroot-video
~~~
- Création du conteneur:
~~~shell script
$ docker run -it pblottiere/embsys-rpi3-buildroot-video /bin/bash
~~~

- Génération de l'éxécutable
~~~
docker$ cd /root
docker$ tar zxvf buildroot-precompiled-2017.08.tar.gz
docker$ git clone https://github.com/KevinBdn/rpi_camera_ip.git
docker$ apt-get update
docker$ apt-get install cmake
docker$ cd rpi_camera_ip
docker$ cd Server
docker$ cmake .
docker$ make
~~~