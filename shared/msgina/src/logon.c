#include <glib.h>
#include <lightdm.h>
#include <wintc-comgtk.h>

#include "challenge.h"
#include "logon.h"

//
// PRIVATE ENUMS
//
enum
{
    SIGNAL_ATTEMPT_COMPLETE = 0,
    N_SIGNALS
};

//
// STATIC DATA
//
static gint wintc_gina_logon_session_signals[N_SIGNALS] = { 0 };

//
// GLIB OOP CLASS/INSTANCE DEFINITIONS
//
struct _WinTCGinaLogonSessionClass
{
    GObjectClass __parent__;
};

struct _WinTCGinaLogonSession
{
    GObject __parent__;

    gboolean        auth_complete;
    gboolean        auth_ready;
    LightDMGreeter* greeter;
    gchar*          response_pwd;
};

//
// FORWARD DECLARATIONS
//
static void on_greeter_authentication_complete(
    LightDMGreeter* greeter,
    gpointer        user_data
);

static void on_greeter_show_prompt(
    LightDMGreeter* greeter,
    gchar*          text,
    WINTC_UNUSED(LightDMPromptType type),
    gpointer        user_data
);

//
// GLIB TYPE DEFINITIONS & CTORS
//
G_DEFINE_TYPE(
    WinTCGinaLogonSession,
    wintc_gina_logon_session,
    G_TYPE_OBJECT
)

static void wintc_gina_logon_session_class_init(
    WinTCGinaLogonSessionClass* klass
)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);

    wintc_gina_logon_session_signals[SIGNAL_ATTEMPT_COMPLETE] =
        g_signal_new(
            "attempt-complete",
            G_TYPE_FROM_CLASS(object_class),
            G_SIGNAL_RUN_FIRST,
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__INT,
            G_TYPE_NONE,
            1,
            G_TYPE_INT
        );
}

static void wintc_gina_logon_session_init(
    WinTCGinaLogonSession* self
)
{
    self->auth_ready = FALSE;
    self->greeter    = NULL;
}

//
// PUBLIC FUNCTIONS
//
WinTCGinaLogonSession* wintc_gina_logon_session_new()
{
    return WINTC_GINA_LOGON_SESSION(
        g_object_new(
            TYPE_WINTC_GINA_LOGON_SESSION,
            NULL
        )
    );
}

gboolean wintc_gina_logon_session_establish(
    WinTCGinaLogonSession* logon_session
)
{
    WINTC_LOG_DEBUG("GINA - establish()");

    if (!logon_session->greeter)
    {
        WINTC_LOG_DEBUG("GINA - new lightdm greeter");

        logon_session->greeter = lightdm_greeter_new();

        g_signal_connect(
            logon_session->greeter,
            "authentication-complete",
            G_CALLBACK(on_greeter_authentication_complete),
            logon_session
        );
        g_signal_connect(
            logon_session->greeter,
            "show-prompt",
            G_CALLBACK(on_greeter_show_prompt),
            logon_session
        );
    }

    WINTC_LOG_DEBUG("GINA - lightdm connect sync");

    if (
        !lightdm_greeter_connect_to_daemon_sync(
            logon_session->greeter,
            NULL
        )
    )
    {
        g_critical("%s", "Failed to connect to LightDM daemon.");
        return FALSE;
    }

    // FIXME: Error handling
    //
    lightdm_greeter_authenticate(
        logon_session->greeter,
        NULL,
        NULL
    );

    return TRUE;
}

void wintc_gina_logon_session_finish(
    WinTCGinaLogonSession* logon_session
)
{
    WINTC_LOG_DEBUG("GINA - finish requested");

    if (!logon_session->auth_complete)
    {
        g_warning("%s", "Attempt to complete incomplete logon.");
        return;
    }

    // FIXME: Error handling required
    // FIXME: If this fails, we need to restart auth! The UI
    //        does not handle this either
    //
    lightdm_greeter_start_session_sync(
        logon_session->greeter,
        NULL,
        NULL
    );
}

gboolean wintc_gina_logon_session_is_available(
    WinTCGinaLogonSession* logon_session
)
{
    WINTC_LOG_DEBUG("GINA - polled");

    return logon_session->auth_ready;
}

void wintc_gina_logon_session_try_logon(
    WinTCGinaLogonSession* logon_session,
    const gchar* username,
    const gchar* password
)
{
    WINTC_LOG_DEBUG("GINA - logon attempt requested");

    if (logon_session->auth_complete)
    {
        g_warning("%s", "Attempt to logon a second time.");
        return;
    }

    logon_session->response_pwd = g_strdup(password);

    // FIXME: Error handling
    //
    lightdm_greeter_respond(
        logon_session->greeter,
        username,
        NULL
    );
}

//
// CALLBACKS
//
static void on_greeter_authentication_complete(
    LightDMGreeter* greeter,
    gpointer        user_data
)
{
    WinTCGinaLogonSession* logon_session =
        WINTC_GINA_LOGON_SESSION(user_data);

    WINTC_LOG_DEBUG("GINA - lightdm auth complete");

    logon_session->auth_complete =
        lightdm_greeter_get_is_authenticated(greeter);

    g_signal_emit(
        logon_session,
        wintc_gina_logon_session_signals[SIGNAL_ATTEMPT_COMPLETE],
        0,
        logon_session->auth_complete ?
            WINTC_GINA_RESPONSE_OKAY :
            WINTC_GINA_RESPONSE_FAIL
    );

    // If the auth failed, have to restart
    //
    if (!logon_session->auth_complete)
    {
        // FIXME: Error handling
        //
        lightdm_greeter_authenticate(greeter, NULL, NULL);
    }
}

static void on_greeter_show_prompt(
    LightDMGreeter* greeter,
    gchar*          text,
    WINTC_UNUSED(LightDMPromptType type),
    gpointer        user_data
)
{
    WinTCGinaLogonSession* logon_session =
        WINTC_GINA_LOGON_SESSION(user_data);

    WINTC_LOG_DEBUG("GINA - lightdm prompt: %s", text);

    if (g_strcmp0(text, "login:") == 0)
    {
        logon_session->auth_ready = TRUE;
    }
    else if (g_strcmp0(text, "Password: ") == 0)
    {
        // FIXME: Error handling
        //
        lightdm_greeter_respond(
            greeter,
            logon_session->response_pwd,
            NULL
        );

        g_free(logon_session->response_pwd);
        logon_session->response_pwd = NULL;
    }
    else
    {
        g_critical("Unknown prompt: %s", text);
    }
}