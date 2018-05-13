/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2018 Ivan Avalos <ivan.avalos.diaz@hotmail.com>
 * 
 * g-quick-cryptor is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * g-quick-cryptor is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gtk/gtk.h>


#include <glib/gi18n.h>

#include "fileman.h"
#include "quickman.h"

typedef struct _Private Private;
struct _Private
{
	/* ANJUTA: Widgets declaration for g_quick_cryptor.ui - DO NOT REMOVE */
	 GtkWindow* window;
	 GtkAboutDialog* adDialog;
	 
	 GtkWidget* sbSize;
	 
	 GtkWidget* fcInput;
	 GtkWidget* fcKey;
	 GtkWidget* fcOutput;
	 GtkWidget* fcGenerateKey;
};

static Private* priv = NULL;

/* For testing purpose, define TEST to use the local (not installed) ui file */
#define TEST
#ifdef TEST
#define UI_FILE "src/g_quick_cryptor.ui"
#else
#ifdef G_OS_WIN32
#define UI_FILE ui_file
#else
#define UI_FILE PACKAGE_DATA_DIR"/ui/g_quick_cryptor.ui"
#endif
#endif
#define TOP_WINDOW "window"

gchar* input_path = NULL;
gboolean input_flag = FALSE;
gchar* key_path = NULL;
gboolean key_flag = FALSE;
gchar* output_path = NULL;
gboolean output_flag = FALSE;
gchar* gen_path = NULL;
gboolean gen_flag = FALSE;

enum {
	OP_ENCRYPT = 0,
	OP_DECRYPT = 1
};

void
show_msg_as_dialog (gchar* title,
                    gchar* msg,
                    GtkMessageType type)
{

	GtkWidget* dialog;
	GtkDialogFlags flags;
	
	flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_message_dialog_new(priv->window,
	                                flags,
	                                type,
	                                GTK_BUTTONS_OK,
	                                msg);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

void
quick_io (gint operation)
{
	if (!input_flag || !key_flag || !output_flag)
	{
		show_msg_as_dialog ("Error", "Input, key and output file are mandatory.",
		                    GTK_MESSAGE_ERROR);
		return;
	}

	fileman_t* io_fileman, *key_fileman;
	char* output = NULL;
	size_t output_size = 0;

	/* Create fileman instances */
	io_fileman = fileman_new (input_path, output_path);
	key_fileman = fileman_new_empty ();
	fileman_set_input_path (key_fileman, key_path);

	/* Read inputs */
	if (fileman_read_input (io_fileman) == FERRNO_FILE)
	{
		show_msg_as_dialog ("Error", "Could not read input file.",
		                    GTK_MESSAGE_ERROR);
		return;
	}

	if (fileman_read_input (key_fileman) == FERRNO_FILE)
	{
		show_msg_as_dialog ("Error", "Could not read key file.",
		                    GTK_MESSAGE_ERROR);
		return;
	}

	if (operation == OP_ENCRYPT)
	{
		/* Cipher input with key */
		output = quickman_xor_cipher (io_fileman->input_content,
			                          io_fileman->input_size,
			                          key_fileman->input_content,
			                          key_fileman->input_size);
		
		output_size = io_fileman->input_size * 2;
	}
	else if (operation == OP_DECRYPT)
	{
		/* Decipher input with key */
		output = quickman_xor_decipher (io_fileman->input_content,
			                            io_fileman->input_size,
			                            key_fileman->input_content,
			                            key_fileman->input_size);

		output_size = io_fileman->input_size / 2;
	}

	/* Check error */
	if (output == NULL) 
	{
		show_msg_as_dialog ("Error", "Key needs to be larger or equal than input file.",
		                    GTK_MESSAGE_ERROR);
		return;
	}
	
	/* Write to output */
	if (fileman_write_output (io_fileman, output,
	                          output_size) == FERRNO_FILE) {
		show_msg_as_dialog ("Error", "Could not write cipher to file",
		                    GTK_MESSAGE_ERROR);
		return;
	}

	/* Success message */
	if (operation == OP_ENCRYPT)
	{
		show_msg_as_dialog ("Success", "File was successfully encrypted",
		                	GTK_MESSAGE_INFO);
	}
	else if (operation == OP_DECRYPT)
	{
		show_msg_as_dialog ("Success", "File was successfully decrypted",
		                	GTK_MESSAGE_INFO);
	}

	/* Free memory */
	fileman_free (io_fileman);
	fileman_free (key_fileman);
	
}

void
quick_gen ()
{
	if (!gen_flag)
	{
		show_msg_as_dialog ("Error", "Output file is mandatory.",
		                    GTK_MESSAGE_ERROR);
		return;
	}
	
	fileman_t* fileman;
	char* key;
	gint s;

	/* Create fileman instance */
	fileman = fileman_new_empty ();
	fileman_set_output_path (fileman, gen_path);

	/* Get key size */
	s = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(priv->sbSize));
	
	/* Generate key */
	key = quickman_key_generate (s);

	/* Write to output */
	if (fileman_write_output (fileman, key, s*2) == FERRNO_FILE) {
		show_msg_as_dialog ("Error", "Could not write key to file.",
		                    GTK_MESSAGE_ERROR);
		return;
	}

	/* Success message */
	show_msg_as_dialog ("Success", "Key was successfully generated!",
		                	GTK_MESSAGE_INFO);
	
	/* Free memory */
	fileman_free (fileman);
	free (key);
}

/* Signal handlers */
/* Note: These may not be declared static because signal autoconnection
 * only works with non-static methods
 */

void
on_fcInput_file_set (GtkFileChooserButton *filechooserbutton, gpointer user_data)
{
	gchar* input_uri;
	input_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (filechooserbutton));
	
	input_path = g_filename_from_uri (input_uri, NULL, NULL);

	input_flag = TRUE;
}

void
on_fcKey_file_set (GtkFileChooserButton *filechooserbutton, gpointer user_data)
{
	gchar* key_uri;
	key_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (filechooserbutton));

	key_path = g_filename_from_uri (key_uri, NULL, NULL);

	key_flag = TRUE;
}

void
on_fcOutput_file_set (GtkFileChooserButton *filechooserbutton, gpointer user_data)
{
	gchar* output_uri;
	output_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (filechooserbutton));

	output_path = g_filename_from_uri (output_uri, NULL, NULL);

	output_flag = TRUE;
}

void
on_fcGenerateKey_file_set (GtkFileChooserButton *filechooserbutton, gpointer user_data)
{
	gchar* gen_uri;
	gen_uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (filechooserbutton));

	gen_path = g_filename_from_uri (gen_uri, NULL, NULL);

	gen_flag = TRUE;
}

void
on_btnEncrypt_clicked (GtkButton *button, gpointer user_data)
{
	quick_io (OP_ENCRYPT);
}

void
on_btnDecrypt_clicked (GtkButton *button, gpointer user_data)
{
	quick_io (OP_DECRYPT);
}

void
on_btnClear_clicked (GtkButton *button, gpointer user_data)
{
	free (input_path);
	input_flag = FALSE;
	gtk_file_chooser_unselect_uri (GTK_FILE_CHOOSER(priv->fcInput),
	                               gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(priv->fcInput)));
	
	free (key_path);
	key_flag = FALSE;
	gtk_file_chooser_unselect_uri (GTK_FILE_CHOOSER(priv->fcKey),
	                               gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(priv->fcKey)));
	
	free (output_path);
	output_flag = FALSE;
	gtk_file_chooser_unselect_uri (GTK_FILE_CHOOSER(priv->fcOutput),
	                               gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(priv->fcOutput)));
	
	free (gen_path);
	gen_flag = FALSE;
	gtk_file_chooser_unselect_uri (GTK_FILE_CHOOSER(priv->fcGenerateKey),
	                               gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(priv->fcGenerateKey)));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->sbSize), (gdouble) 0.0f);
}

void
on_btnGenerateKey_clicked (GtkButton *button, gpointer user_data)
{
	quick_gen ();
}

void
on_btnQuit_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_main_quit ();
}

void
on_btnAbout_activate (GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_window_set_transient_for (GTK_WINDOW (priv->adDialog), priv->window);
	gtk_widget_show (GTK_WIDGET (priv->adDialog));
}

void
on_adAbout_close (GtkDialog *dialog, gpointer user_data)
{
	gtk_widget_hide (GTK_WIDGET (priv->adDialog));
}


/* Called when the window is closed */
void
on_window_destroy (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

static GtkWidget*
create_window (void)
{
#if !defined(TEST) && defined(G_OS_WIN32)
	gchar *prefix = g_win32_get_package_installation_directory_of_module (NULL);
	gchar *datadir = g_build_filename (prefix, "share", PACKAGE, NULL);
	gchar *ui_file = g_build_filename (datadir, "ui", "g_quick_cryptor.ui", NULL);
#endif
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;

	/* Load UI from file */
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, UI_FILE, &error))
	{
		g_critical ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	/* Auto-connect signal handlers */
	gtk_builder_connect_signals (builder, NULL);

	/* Get the window object from the ui file */
	window = GTK_WIDGET (gtk_builder_get_object (builder, TOP_WINDOW));
        if (!window)
        {
                g_critical ("Widget \"%s\" is missing in file %s.",
				TOP_WINDOW,
				UI_FILE);
        }

	priv = g_malloc (sizeof (struct _Private));
	/* ANJUTA: Widgets initialization for g_quick_cryptor.ui - DO NOT REMOVE */
	priv->window = GTK_WINDOW (window);
	priv->adDialog = GTK_ABOUT_DIALOG (gtk_builder_get_object (builder, "adAbout"));
	
	priv->sbSize = GTK_WIDGET (gtk_builder_get_object (builder, "sbSize"));
	
	priv->fcInput = GTK_WIDGET (gtk_builder_get_object (builder, "fcInput"));
	priv->fcKey = GTK_WIDGET (gtk_builder_get_object (builder, "fcKey"));
	priv->fcOutput = GTK_WIDGET (gtk_builder_get_object (builder, "fcOutput"));
	priv->fcGenerateKey = GTK_WIDGET (gtk_builder_get_object (builder, "fcGenerateKey"));

	g_object_unref (builder);


#if !defined(TEST) && defined(G_OS_WIN32)
	g_free (prefix);
	g_free (datadir);
	g_free (ui_file);
#endif
	
	return window;
}

int
main (int argc, char *argv[])
{
 	GtkWidget *window;


#ifdef G_OS_WIN32
	gchar *prefix = g_win32_get_package_installation_directory_of_module (NULL);
	gchar *localedir = g_build_filename (prefix, "share", "locale", NULL);
#endif

#ifdef ENABLE_NLS

# ifndef G_OS_WIN32
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
# else
	bindtextdomain (GETTEXT_PACKAGE, localedir);
# endif
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	gtk_init (&argc, &argv);

	window = create_window ();
	gtk_widget_show (window);

	gtk_main ();


	g_free (priv);

#ifdef G_OS_WIN32
	g_free (prefix);
	g_free (localedir);
#endif

	return 0;
}

