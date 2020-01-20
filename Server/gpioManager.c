#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define LOW 0
#define HIGH 1

typedef struct GPIO{
    int fd;
    int num;
} GPIO;


GPIO* initGPIO(int num, char input_output_mode[3])
{
    /*      Initialization of the GPIO using sysfs interface        */
    /*-------                                                       */
    /*      Return: GPIO*                                           */
    /*            - `fd` : the value FileDescriptor of the gpio     */
    /*            - `num`: the GPIO number                          */

    GPIO * gpio = malloc(sizeof(GPIO));
    char pin_number[2];
    int fd;
    char path[40];


    sprintf(pin_number,"%d",num);
    sprintf(path,"/sys/class/gpio/gpio%d/direction",num);

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/export");
        exit(1);
    }
    if (write(fd, pin_number, 2) != 2) {
        perror("Error writing to /sys/class/gpio/export");
        exit(1);
    }
    close(fd);

    sprintf(path,"/sys/class/gpio/gpio%d/direction",num);
    fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/gpioXXX/direction");
        exit(1);
    }

    if (write(fd, input_output_mode, strlen(input_output_mode)) != strlen(input_output_mode)) {
        perror("Error writing to /sys/class/gpio/gpioXXX/direction");
        exit(1);
    }

    sprintf(path,"/sys/class/gpio/gpio%d/value",num);

    fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/gpioXXX/value");
        exit(1);
    }

    gpio->fd=fd;
    gpio->num=num;
    return gpio;
}



int writeGPIO(GPIO* gpio, int val)
{

    /*   Change the level of a GPIO number using sysfs interface    */
    /*-------                                                       */
    /*      Return: The new level                                   */



    char level[1];

    sprintf(level,"%d",val);

    if (write(gpio->fd, level, 1) != 1) {
        perror("Error writing to /sys/class/gpio/gpioXXX/value");
        exit(1);
    }

    return val;

}



int closeGPIO(GPIO* gpio)
{

    /*   Close a GPIO using sysfs interface    */
    /*------                                   */
    /*      Return: 1 if success               */


    int fd;
    char pin_number[2];

    sprintf(pin_number,"%d",gpio->num);
    fd = open("/sys/class/gpio/unexport", O_WRONLY);


    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/unexport");
        exit(1);
    }

    if (write(fd, pin_number, 2) != 2) {
        perror("Error writing to /sys/class/gpio/unexport");
        exit(1);
    }

    close(fd);
    close(gpio->fd);
    free(gpio);
    return 1;
}

int main()
{

    GPIO* gpio;
    int val=0;

    gpio = initGPIO(24, "out");

    for (int i; i<20; i++)
    {
        val = writeGPIO(gpio, !val);
        sleep(1);
    }

    closeGPIO(gpio);



    // And exit
    return 0;
}
