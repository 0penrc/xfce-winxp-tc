#ifndef __WELCOME_UI_H__
#define __WELCOME_UI_H__

#include <glib.h>
#include <gtk/gtk.h>

//
// GTK OOP BOILERPLATE
//
typedef struct _WinTCWelcomeUIPrivate WinTCWelcomeUIPrivate;
typedef struct _WinTCWelcomeUIClass   WinTCWelcomeUIClass;
typedef struct _WinTCWelcomeUI        WinTCWelcomeUI;

#define TYPE_WINTC_WELCOME_UI            (wintc_welcome_ui_get_type())
#define WINTC_WELCOME_UI(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_WINTC_WELCOME_UI, WinTCWelcomeUI))
#define WINTC_WELCOME_UI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_WINTC_WELCOME_UI, WinTCWelcomeUI))
#define IS_WINTC_WELCOME_UI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_WINTC_WELCOME_UI))
#define IS_WINTC_WELCOME_UI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_WINTC_WELCOME_UI))
#define WINTC_WELCOME_UI_GET_CLASS(obj)  (G_TYPE_CHECK_INSTANCE_GET_CLASS((obj), TYPE_WINTC_WELCOME_UI))

GType wintc_welcome_ui_get_type(void) G_GNUC_CONST;

//
// PUBLIC FUNCTIONS
//
GtkWidget* wintc_welcome_ui_new(void);

#endif
