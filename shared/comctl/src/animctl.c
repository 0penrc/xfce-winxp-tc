#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <wintc-comgtk.h>

#include "animctl.h"

#define ONE_SECOND_IN_US 1000000

//
// PRIVATE ENUMS
//
enum
{
    PROP_HALIGN = 1,
    PROP_VALIGN,
    N_PROPERTIES
};

//
// PRIVATE STURCTURE DEFINITIONS
//
typedef struct _WinTCAnimationData
{
    GdkPixbuf*       pixbuf_bmp;
    cairo_surface_t* surface_bmp;

    gint frame_count;
    gint frame_height;
} WinTCAnimationData;

//
// GTK OOP CLASS/INSTANCE DEFINITIONS
//
struct _WinTCAnimationPrivate
{
    GSList* animations;

    gboolean is_animating;
    guint    tick_id;

    WinTCAnimationData* current_anim;
    gint                current_frame;
    gint                desired_repeats;
    gint                last_frame;
    gint64              origin_frame_time;
    gint64              per_frame_time;
    gint64              playback_total_time;

    GtkAlign halign;
    GtkAlign valign;
};

struct _WinTCAnimationClass
{
    GtkWidgetClass __parent__;
};

struct _WinTCAnimation
{
    GtkWidget __parent__;

    WinTCAnimationPrivate* priv;
};

//
// FORWARD DECLARATIONS
//
static void wintc_animation_finalize(
    GObject* gobject
);
static void wintc_animation_get_property(
    GObject*    object,
    guint       prop_id,
    GValue*     value,
    GParamSpec* pspec
);
static void wintc_animation_set_property(
    GObject*      object,
    guint         prop_id,
    const GValue* value,
    GParamSpec*   pspec
);

static gboolean wintc_animation_draw(
    GtkWidget* widget,
    cairo_t*   cr
);
static void wintc_animation_get_preferred_height(
    GtkWidget* widget,
    gint*      minimum_height,
    gint*      natural_height
);
static void wintc_animation_get_preferred_height_for_width(
    GtkWidget* widget,
    gint       width,
    gint*      minimum_height,
    gint*      natural_height
);
static void wintc_animation_get_preferred_width(
    GtkWidget* widget,
    gint*      minimum_width,
    gint*      natural_width
);
static void wintc_animation_get_preferred_width_for_height(
    GtkWidget* widget,
    gint       height,
    gint*      minimum_width,
    gint*      natural_width
);

static void free_anim_data(
    WinTCAnimationData* anim_data
);

static gboolean wintc_animation_step(
    GtkWidget*     widget,
    GdkFrameClock* frame_clock,
    gpointer       user_data
);

//
// GTK TYPE DEFINITIONS & CTORS
//
G_DEFINE_TYPE_WITH_CODE(
    WinTCAnimation,
    wintc_animation,
    GTK_TYPE_WIDGET,
    G_ADD_PRIVATE(WinTCAnimation)
)

static void wintc_animation_class_init(
    WinTCAnimationClass* klass
)
{
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
    GObjectClass*   object_class = G_OBJECT_CLASS(klass);

    object_class->finalize     = wintc_animation_finalize;
    object_class->get_property = wintc_animation_get_property;
    object_class->set_property = wintc_animation_set_property;

    widget_class->draw                           =
        wintc_animation_draw;
    widget_class->get_preferred_height           =
        wintc_animation_get_preferred_height;
    widget_class->get_preferred_height_for_width =
        wintc_animation_get_preferred_height_for_width;
    widget_class->get_preferred_width            =
        wintc_animation_get_preferred_width;
    widget_class->get_preferred_width_for_height =
        wintc_animation_get_preferred_width_for_height;

    g_object_class_install_property(
        object_class,
        PROP_HALIGN,
        g_param_spec_enum(
            "gfx-halign",
            "GfxHAlign",
            "The horizontal alignment for drawing.",
            GTK_TYPE_ALIGN,
            GTK_ALIGN_CENTER,
            G_PARAM_WRITABLE
        )
    );
    g_object_class_install_property(
        object_class,
        PROP_VALIGN,
        g_param_spec_enum(
            "gfx-valign",
            "GfxVAlign",
            "The vertical alignment for drawing.",
            GTK_TYPE_ALIGN,
            GTK_ALIGN_CENTER,
            G_PARAM_WRITABLE
        )
    );
}

static void wintc_animation_init(
    WinTCAnimation* self
)
{
    self->priv = wintc_animation_get_instance_private(self);

    gtk_widget_set_has_window(GTK_WIDGET(self), FALSE);
}

//
// CLASS VIRTUAL METHODS
//
static void wintc_animation_finalize(
    GObject* gobject
)
{
    WinTCAnimation* anim = WINTC_ANIMATION(gobject);

    g_slist_free_full(
        anim->priv->animations,
        (GDestroyNotify) free_anim_data
    );

    if (anim->priv->tick_id > 0)
    {
        gtk_widget_remove_tick_callback(
            GTK_WIDGET(anim),
            anim->priv->tick_id
        );
    }

    (G_OBJECT_CLASS(wintc_animation_parent_class))->finalize(gobject);
}

static void wintc_animation_get_property(
    GObject*    object,
    guint       prop_id,
    GValue*     value,
    GParamSpec* pspec
)
{
    WinTCAnimation* anim = WINTC_ANIMATION(object);

    switch (prop_id)
    {
        case PROP_HALIGN:
            g_value_set_enum(value, anim->priv->halign);
            break;

        case PROP_VALIGN:
            g_value_set_enum(value, anim->priv->valign);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void wintc_animation_set_property(
    GObject*      object,
    guint         prop_id,
    const GValue* value,
    GParamSpec*   pspec
)
{
    WinTCAnimation* anim = WINTC_ANIMATION(object);

    switch (prop_id)
    {
        case PROP_HALIGN:
            anim->priv->halign = g_value_get_enum(value);
            gtk_widget_queue_draw(GTK_WIDGET(object));
            break;

        case PROP_VALIGN:
            anim->priv->valign = g_value_get_enum(value);
            gtk_widget_queue_draw(GTK_WIDGET(object));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static gboolean wintc_animation_draw(
    GtkWidget* widget,
    cairo_t*   cr
)
{
    WinTCAnimation* anim = WINTC_ANIMATION(widget);

    if (anim->priv->current_anim == NULL)
    {
        return FALSE;
    }

    // Set up to render the graphic
    //
    gint   graphic_height;
    gint   graphic_width;
    double y_offset;

    graphic_width =
        cairo_image_surface_get_width(
            anim->priv->current_anim->surface_bmp
        );

    if (anim->priv->current_anim->frame_count > 1)
    {
        graphic_height = anim->priv->current_anim->frame_height;
        y_offset       = (graphic_height * anim->priv->current_frame) * -1.0f;
    }
    else
    {
        graphic_height =
            cairo_image_surface_get_height(
                anim->priv->current_anim->surface_bmp
            );
        y_offset       = 0.0f;
    }

    // Calculate where we're drawing...
    //
    gint my_height = gtk_widget_get_allocated_height(widget);
    gint my_width  = gtk_widget_get_allocated_width(widget);

    double target_x;
    double target_y;
    double target_width   = graphic_width;
    double target_height  = graphic_height;
    double target_scale_w = 1.0f;
    double target_scale_h = 1.0f;

    switch (anim->priv->halign)
    {
        case GTK_ALIGN_END:
            target_x = (double) (my_width - graphic_width);
            break;

        case GTK_ALIGN_CENTER:
            target_x = (double) ((my_width / 2) - (graphic_width / 2));
            break;

        case GTK_ALIGN_FILL:
            target_width   = my_width;
            target_scale_w = (double) my_width / (double) graphic_width;
            // Fall-through

        default:
            target_x = 0.0f;
            break;
    }

    switch (anim->priv->valign)
    {
        case GTK_ALIGN_END:
            target_y = (double) (my_height - graphic_height);
            break;

        case GTK_ALIGN_CENTER:
            target_y = (double) ((my_height / 2) - (graphic_height / 2));
            break;

        case GTK_ALIGN_FILL:
            target_height  = my_height;
            target_scale_h = (double) my_height / (double) graphic_height;
            // Fall-through

        default:
            target_y = 0.0f;
            break;
    }

    // Draw now
    //
    cairo_save(cr);

    cairo_translate(cr, target_x, target_y);

    cairo_rectangle(
        cr,
        0.0f,
        0.0f,
        (double) target_width,
        (double) target_height
    );
    cairo_clip(cr);

    cairo_scale(cr, target_scale_w, target_scale_h);

    cairo_set_source_surface(
        cr,
        anim->priv->current_anim->surface_bmp,
        0.0f,
        y_offset
    );
    cairo_pattern_set_extend(
        cairo_get_source(cr),
        CAIRO_EXTEND_NONE
    );

    cairo_paint(cr);

    cairo_restore(cr);

    anim->priv->last_frame = anim->priv->current_frame;

    return FALSE;
}

static void wintc_animation_get_preferred_height(
    GtkWidget* widget,
    gint*      minimum_height,
    gint*      natural_height
)
{
    WinTCAnimation* anim   = WINTC_ANIMATION(widget);
    gint            height = 0;

    if (anim->priv->current_anim != NULL)
    {
        if (anim->priv->current_anim->frame_count == 1)
        {
            height =
                cairo_image_surface_get_height(
                    anim->priv->current_anim->surface_bmp
                );
        }
        else
        {
            height = anim->priv->current_anim->frame_height;
        }
    }

    *minimum_height = height;
    *natural_height = height;
}

static void wintc_animation_get_preferred_height_for_width(
    GtkWidget* widget,
    WINTC_UNUSED(gint width),
    gint*      minimum_height,
    gint*      natural_height
)
{
    wintc_animation_get_preferred_height(
        widget,
        minimum_height,
        natural_height
    );
}

static void wintc_animation_get_preferred_width(
    GtkWidget* widget,
    gint*      minimum_width,
    gint*      natural_width
)
{
    WinTCAnimation* anim = WINTC_ANIMATION(widget);

    if (anim->priv->current_anim == NULL)
    {
        minimum_width = 0;
        natural_width = 0;
        return;
    }

    gint width =
        cairo_image_surface_get_width(
            anim->priv->current_anim->surface_bmp
        );

    *minimum_width = width;
    *natural_width = width;
}

static void wintc_animation_get_preferred_width_for_height(
    GtkWidget* widget,
    WINTC_UNUSED(gint height),
    gint*      minimum_width,
    gint*      natural_width
)
{
    wintc_animation_get_preferred_width(
        widget,
        minimum_width,
        natural_width
    );
}

//
// PUBLIC FUNCTIONS
//
GtkWidget* wintc_animation_new(void)
{
    return GTK_WIDGET(
        g_object_new(
            TYPE_WINTC_ANIMATION,
            NULL
        )
    );
}

guint wintc_animation_add_framesheet(
    WinTCAnimation* anim,
    GdkPixbuf*      framesheet_pixbuf,
    gint            frame_count
)
{
    WinTCAnimationData* anim_data = g_new0(WinTCAnimationData, 1);
    gint                height    = gdk_pixbuf_get_height(framesheet_pixbuf);

    g_object_ref(framesheet_pixbuf);

    anim_data->pixbuf_bmp  = framesheet_pixbuf;
    anim_data->surface_bmp =
        gdk_cairo_surface_create_from_pixbuf(
            framesheet_pixbuf,
            1,
            NULL // FIXME: Error reporting
        );

    anim_data->frame_count  = frame_count;
    anim_data->frame_height = height / frame_count;

    anim->priv->animations =
        g_slist_append(
            anim->priv->animations,
            anim_data
        );

    return wintc_animation_get_count(anim);
}

guint wintc_animation_add_static(
    WinTCAnimation* anim,
    GdkPixbuf*      static_pixbuf
)
{
    WinTCAnimationData* anim_data = g_new0(WinTCAnimationData, 1);

    g_object_ref(static_pixbuf);

    anim_data->pixbuf_bmp  = static_pixbuf;
    anim_data->surface_bmp =
        gdk_cairo_surface_create_from_pixbuf(
            static_pixbuf,
            1,
            NULL // FIXME: Error reporting
        );

    anim_data->frame_count  = 1;
    anim_data->frame_height = 0;

    anim->priv->animations =
        g_slist_append(
            anim->priv->animations,
            anim_data
        );

    return wintc_animation_get_count(anim);
}

guint wintc_animation_get_count(
    WinTCAnimation* anim
)
{
    return g_slist_length(anim->priv->animations);
}

GtkAlign wintc_animation_get_halign(
    WinTCAnimation* anim
)
{
    GtkAlign value;

    g_object_get(anim, "gfx-halign", &value, NULL);

    return value;
}

GtkAlign wintc_animation_get_valign(
    WinTCAnimation* anim
)
{
    GtkAlign value;

    g_object_get(anim, "gfx-valign", &value, NULL);

    return value;
}

void wintc_animation_play(
    WinTCAnimation* anim,
    guint           id,
    gint            frame_rate,
    gint            repeats
)
{
    // Reset anim state values
    //
    anim->priv->is_animating = FALSE;

    anim->priv->current_anim        = NULL;
    anim->priv->current_frame       = 0;
    anim->priv->last_frame          = 0;
    anim->priv->desired_repeats     = 0;
    anim->priv->origin_frame_time   = 0;
    anim->priv->per_frame_time      = 0;
    anim->priv->playback_total_time = 0;

    // Queue a draw regardless of what happens
    //
    gtk_widget_queue_draw(GTK_WIDGET(anim));

    // Check we're being asked to actually play anything
    //
    if (id > wintc_animation_get_count(anim))
    {
        g_warning("WinTCAnimation - attempted to play invalid ID %u", id);
        return;
    }

    if (id == WINTC_ANIMATION_NONE)
    {
        return;
    }

    // Wahey! We have been asked to play something!
    //
    gint real_id = id - 1; // The consumer is given the count rather than index
                           // in the 'add' APIs, so that we can have a special
                           // meaning for 0 (WINTC_ANIMATION_NONE)

    GSList*             anim_link = g_slist_nth(
                                        anim->priv->animations,
                                        real_id
                                    );
    WinTCAnimationData* anim_data = anim_link->data;

    anim->priv->current_anim = anim_data;

    if (
        anim_data->frame_count > 1 &&
        frame_rate             > 0
    )
    {
        anim->priv->is_animating = TRUE;

        anim->priv->desired_repeats     = repeats;
        anim->priv->origin_frame_time   = g_get_monotonic_time();
        anim->priv->per_frame_time      = ONE_SECOND_IN_US / frame_rate;
        anim->priv->playback_total_time =
            anim->priv->per_frame_time * anim_data->frame_count;

        if (anim->priv->tick_id == 0)
        {
            gtk_widget_add_tick_callback(
                GTK_WIDGET(anim),
                (GtkTickCallback) wintc_animation_step,
                NULL,
                NULL
            );
        }
    }
}

void wintc_animation_remove(
    WinTCAnimation* anim,
    guint           id
)
{
    if (id == 0 || id > wintc_animation_get_count(anim))
    {
        g_warning("WinTCAnimation - attempted to remove invalid ID %u", id);
        return;
    }

    gint    real_id   = id - 1; 
    GSList* to_delete = g_slist_nth(anim->priv->animations, real_id);

    free_anim_data(to_delete->data);

    anim->priv->animations =
        g_slist_delete_link(
            anim->priv->animations,
            to_delete
        );
}

void wintc_animation_set_halign(
    WinTCAnimation* anim,
    GtkAlign        align
)
{
    g_object_set(anim, "gfx-halign", align, NULL);
}

void wintc_animation_set_valign(
    WinTCAnimation* anim,
    GtkAlign        align
)
{
    g_object_set(anim, "gfx-valign", align, NULL);
}

//
// PRIVATE FUNCTIONS
//
static void free_anim_data(
    WinTCAnimationData* anim_data
)
{
    cairo_surface_destroy(anim_data->surface_bmp);
    g_clear_object(&(anim_data->pixbuf_bmp));
    g_free(anim_data);
}

//
// CALLBACKS
//
static gboolean wintc_animation_step(
    GtkWidget*     widget,
    GdkFrameClock* frame_clock,
    WINTC_UNUSED(gpointer user_data)
)
{
    WinTCAnimation* anim = WINTC_ANIMATION(widget);

    if (!anim->priv->is_animating)
    {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time   = gdk_frame_clock_get_frame_time(frame_clock);
    gint64 progress     = frame_time - anim->priv->origin_frame_time;
    gint64 repeat_no    = progress / anim->priv->playback_total_time;
    gint64 within_frame = progress % anim->priv->playback_total_time;
    gint   frame_no     = (gint) (within_frame / anim->priv->per_frame_time);

    if (
        anim->priv->desired_repeats != WINTC_ANIMATION_INFINITE &&
        anim->priv->desired_repeats <  repeat_no
    )
    {
        return G_SOURCE_REMOVE;
    }

    if (anim->priv->last_frame != frame_no)
    {
        gtk_widget_queue_draw(widget);
    }

    anim->priv->current_frame = frame_no;

    return G_SOURCE_CONTINUE;
}