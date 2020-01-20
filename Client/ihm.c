/* 
To compile:
gcc -g ihm.c -o IHM  `pkg-config --libs --cflags gtk+-3.0 gmodule-2.0`

Official documentation:
https://developer.gnome.org/gtk3/stable/

*/

#include <gtk/gtk.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define OK 1
#define KO 0 

#define TAKE 1
#define SAVE 2


GtkWidget *main_window = NULL;
GtkWidget *picture = NULL;  

int status = KO;
char IP_CAMERA[30]="192.168.0.1";

void format_time(char *output){
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

/*    sprintf(output, "[%d/%d/%d-%dH:%d':%d'']",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);*/
    sprintf(output, "[ %.2dh%.2dm%.2ds ]", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void updateInfo(GtkLabel * InfLabel, int event, char * name)
{
    char info[1000];
    char state[80];
    char action[256];
    char time[30];

    format_time(time);

    switch(status)
    {
    case OK: sprintf(state," • Camera state: Ready\n\n • IP Address: %s",IP_CAMERA); break;
    case KO: sprintf(state," • Camera state: No connection\n\n • IP Address: No connection"); break;
    }

    switch(event)
    {
    case TAKE: sprintf(action, " • Last event:\n\n\t %s Picture taken", time); break;
    case SAVE: sprintf(action, " • Last event: \n\n\t %s Picture saved\n\t(%.30s)", time, name); break;
    default: sprintf(action, " • Last event:\n\n\t\t No event");
    }

    sprintf(info,"%s\n\n%s\n",state,action);
    gtk_label_set_text(InfLabel,info);
}

int open_dialog(char * name_file)
{
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new("Choose a file:", GTK_WINDOW(main_window), 
                         GTK_FILE_CHOOSER_ACTION_SAVE,
                         ("_Cancel"),
                         GTK_RESPONSE_CANCEL,
                         ("_Save"),
                         GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "image.jpg");
    gtk_widget_show_all(dialog);
    gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
    if (resp == GTK_RESPONSE_OK)
    {
        g_print("\tPath: %s\n", gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
        sprintf(name_file,"%s", gtk_file_chooser_get_current_name(GTK_FILE_CHOOSER(dialog)));
        gtk_widget_destroy(dialog);
        return 1;
    }
    else
    {
        g_print("\tSaving cancel\n");
        gtk_widget_destroy(dialog);
        return 0;
    }
}

void takePicture(GtkWidget *widget, gpointer data)
{
    printf("\nTake picture\n");
    updateInfo((GtkLabel*) data, TAKE, NULL);
/*    gtk_image_set_from_file((GtkImage *)picture,"1.png");*/
/*    gtk_image_new_from_pixbuf ()*/
}


void savePicture(GtkWidget *widget, gpointer data)
{
    printf("\nSave picture\n");
    char name_file[256];
    int success = open_dialog(name_file);
    if (success) updateInfo((GtkLabel*) data, SAVE, name_file);
}

void cameraDetection(GtkWidget *widget, gpointer data)
{
    printf("\nCamera detection started ...\n");
}



int ihm(int argc, char *argv [])
{


    GtkWidget *take_button = NULL;  
    GtkWidget *save_button= NULL;
    GtkWidget *detection_button= NULL;
    GtkWidget *inf_label = NULL;  
    GtkBuilder *builder = NULL;
    gchar *filename = NULL;
    GError *error = NULL;

    /* Initialisation of the Gtk.library */
    gtk_init(&argc, &argv);

    /* Glade file opening*/
    builder = gtk_builder_new();


    /* g_build_filename(): get the path for the file according to the OS */
    filename =  g_build_filename("../Client/IHM.glade", NULL);

    /* Loading of the "ihm.glade" file */
    gtk_builder_add_from_file(builder, filename, &error);
    if (error)
    {
        gint code = error->code;
        g_printerr("%s\n", error->message);
        g_error_free (error);
        return code;
    }

    /* Getting of main window pointer */
    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    if (NULL == main_window)
    {
        /* Print out the error. You can use GLib's message logging  */
        fprintf(stderr, "Unable to file object with id \"window1\" \n");
        /* Your error handling code goes here */
    }
    else
    {
        /* Widget getting */

        take_button = GTK_WIDGET(gtk_builder_get_object(builder, "take_button"));
        save_button = GTK_WIDGET(gtk_builder_get_object(builder, "save_button"));
        detection_button = GTK_WIDGET(gtk_builder_get_object(builder, "detection_button"));
        inf_label = GTK_WIDGET(gtk_builder_get_object(builder, "information_label"));
        picture = GTK_WIDGET(gtk_builder_get_object(builder, "picture"));

        /* Callback function assignment */
        g_signal_connect (G_OBJECT (main_window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
        g_signal_connect(G_OBJECT(save_button), "clicked", G_CALLBACK(savePicture), inf_label);
        g_signal_connect(G_OBJECT(take_button), "clicked", G_CALLBACK(takePicutre), inf_label);
        g_signal_connect(G_OBJECT(detection_button), "clicked", G_CALLBACK(cameraDetection), inf_label);

        /* Display the main window */
        gtk_widget_show_all(main_window);
        g_free (filename);

        updateInfo((GtkLabel*) inf_label, -1, NULL);

        gtk_main();
    }

    return 0;
}

int main(int argc, char *argv [])
{
    ihm(argc, argv);
    return EXIT_SUCCESS;
}
