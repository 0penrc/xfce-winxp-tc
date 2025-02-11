#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <glib.h>
#include <gtk/gtk.h>

//
// GTK OOP BOILERPLATE
//
typedef struct _WinTCLogonUIWindowPrivate WinTCLogonUIWindowPrivate;
typedef struct _WinTCLogonUIWindowClass   WinTCLogonUIWindowClass;
typedef struct _WinTCLogonUIWindow        WinTCLogonUIWindow;

#define TYPE_WINTC_LOGONUI_WINDOW            (wintc_logonui_window_get_type())
#define WINTC_LOGONUI_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_WINTC_LOGONUI_WINDOW, WinTCLogonUIWindow))
#define WINTC_LOGONUI_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), TYPE_WINTC_LOGONUI_WINDOW, WinTCLogonUIWindow))
#define IS_WINTC_LOGONUI_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_WINTC_LOGONUI_WINDOW))
#define IS_WINTC_LOGONUI_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), TYPE_WINTC_LOGONUI_WINDOW))
#define WINTC_LOGONUI_WINDOW_GET_CLASS(obj)  (G_TYPE_CHECK_INSTANCE_GET_CLASS((obj), TYPE_WINTC_LOGONUI_WINDOW))

GType wintc_logonui_window_get_type(void) G_GNUC_CONST;

//
// PUBLIC FUNCTIONS
//
GtkWidget* wintc_logonui_window_new();

#endif
