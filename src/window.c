/*
 * Copyright (c) 2008-2009  Christian Hammond
 * Copyright (c) 2008-2009  David Trowbridge
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "action-list.h"
#include "parasite.h"
#include "prop-list.h"
#include "style-list.h"
#include "widget-tree.h"

#include "config.h"

static void
on_widget_tree_selection_changed(ParasiteWidgetTree *widget_tree,
                                 ParasiteWindow *parasite)
{
    GtkWidget *selected = parasite_widget_tree_get_selected_widget(widget_tree);
    if (selected != NULL) {
        gchar *path;

        parasite_proplist_set_widget(PARASITE_PROPLIST(parasite->prop_list),
                                     selected);
        parasite_style_list_set_widget (PARASITE_STYLE_LIST (parasite->style_list),
                                        selected);

        path = gtk_widget_path_to_string (gtk_widget_get_path (selected));
        gtk_entry_set_text (GTK_ENTRY (parasite->widget_path_entry), path);
        gtk_editable_set_position (GTK_EDITABLE (parasite->widget_path_entry), -1);
        g_free (path);

        /* Flash the widget. */
        gtkparasite_flash_widget(parasite, selected);
    }
}


static GtkWidget *
create_widget_list_pane(ParasiteWindow *parasite)
{
    GtkWidget *swin;

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin),
                                        GTK_SHADOW_IN);

    parasite->widget_tree = parasite_widget_tree_new();
    gtk_widget_show(parasite->widget_tree);
    gtk_container_add(GTK_CONTAINER(swin), parasite->widget_tree);

    g_signal_connect(G_OBJECT(parasite->widget_tree),
                     "widget-changed",
                     G_CALLBACK(on_widget_tree_selection_changed),
                     parasite);

    return swin;
}

static gboolean
style_classes_entry_ratelimiter (gpointer data)
{
  ParasiteWindow *parasite = data;

  parasite_style_list_set_classes (PARASITE_STYLE_LIST (parasite->style_list),
                                   gtk_entry_get_text (GTK_ENTRY (parasite->style_classes_entry)));

  parasite->style_classes_entry_ratelimit_id = 0;

  return 1;
}

static void
style_classes_entry_changed (GtkEditable *editable,
                             gpointer     data)
{
  ParasiteWindow *parasite = data;

  if (parasite->style_classes_entry_ratelimit_id)
    g_source_remove (parasite->style_classes_entry_ratelimit_id);

  parasite->style_classes_entry_ratelimit_id = g_timeout_add_seconds (1,
                                                                      style_classes_entry_ratelimiter,
                                                                      data);
}

static void
style_classes_entry_activate (GtkEntry *entry,
                              gpointer  data)
{
  ParasiteWindow *parasite = data;

  if (parasite->style_classes_entry_ratelimit_id)
    g_source_remove (parasite->style_classes_entry_ratelimit_id);

  style_classes_entry_ratelimiter (data);
}

static GtkWidget *
create_prop_list_pane(ParasiteWindow *parasite)
{
    GtkWidget *notebook;
    GtkWidget *swin;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *vbox, *hbox;

    notebook = gtk_notebook_new ();

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show (swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    parasite->prop_list = parasite_proplist_new();
    gtk_widget_show(parasite->prop_list);
    gtk_container_add(GTK_CONTAINER(swin), parasite->prop_list);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), swin, gtk_label_new ("Widget properties"));

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show (vbox);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, gtk_label_new ("Style properties"));

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_show (hbox);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Extra classes:");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    parasite->style_classes_entry_ratelimit_id = 0;

    parasite->style_classes_entry = entry = gtk_entry_new ();
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
    gtk_widget_set_halign (entry, GTK_ALIGN_CENTER);
    gtk_widget_set_valign (entry, GTK_ALIGN_CENTER);
    g_signal_connect (entry, "changed",
                      G_CALLBACK (style_classes_entry_changed), parasite);
    g_signal_connect (entry, "activate",
                      G_CALLBACK (style_classes_entry_activate), parasite);

    label = gtk_label_new ("Widget path:");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    parasite->widget_path_entry = entry = gtk_entry_new ();
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show (swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    parasite->style_list = parasite_style_list_new ();
    gtk_widget_show (parasite->style_list);
    gtk_container_add (GTK_CONTAINER (swin), parasite->style_list);

    gtk_box_pack_start (GTK_BOX (vbox), swin, TRUE, TRUE, 0);

    return notebook;
}

static void
on_edit_mode_toggled(GtkWidget *toggle_button,
                     ParasiteWindow *parasite)
{
    gboolean active =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button));

    parasite->edit_mode_enabled = active;
    parasite_widget_tree_set_edit_mode(PARASITE_WIDGET_TREE(parasite->widget_tree),
                                       active);
}

static void
on_show_graphic_updates_toggled(GtkWidget *toggle_button,
                                ParasiteWindow *parasite)
{
    gdk_window_set_debug_updates(
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button)));
}

static GtkWidget *
create_widget_tree(ParasiteWindow *parasite)
{
    GtkWidget *vbox;
    GtkWidget *bbox;
    GtkWidget *button;
    GtkWidget *swin;
    GtkWidget *hpaned;

    hpaned = gtk_hpaned_new();
    gtk_widget_show(hpaned);
    gtk_container_set_border_width(GTK_CONTAINER(hpaned), 12);

    vbox = gtk_vbox_new(FALSE, 6);
    gtk_widget_show(vbox);
    gtk_paned_pack1(GTK_PANED(hpaned), vbox, FALSE, FALSE);

    bbox = gtk_hbutton_box_new();
    gtk_widget_show(bbox);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_START);
    gtk_box_set_spacing(GTK_BOX(bbox), 6);

    button = gtkparasite_inspect_button_new(parasite);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);

    button = gtk_toggle_button_new_with_mnemonic("_Edit Mode");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(on_edit_mode_toggled), parasite);

    button = gtk_toggle_button_new_with_mnemonic("_Show Graphic Updates");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(bbox), button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(on_show_graphic_updates_toggled), parasite);

    swin = create_widget_list_pane(parasite);
    gtk_widget_show(swin);
    gtk_box_pack_start (GTK_BOX (vbox), swin, TRUE, TRUE, 0);

    swin = create_prop_list_pane(parasite);
    gtk_widget_show(swin);
    gtk_paned_pack2(GTK_PANED(hpaned), swin, FALSE, FALSE);

    return hpaned;
}

static GtkWidget *
create_action_list(ParasiteWindow *parasite)
{
    GtkWidget *vbox;
    GtkWidget *swin;

    vbox = gtk_vbox_new(FALSE, 6);
    gtk_widget_show(vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(swin);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin),
                                        GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 0);

    parasite->action_list = parasite_actionlist_new(parasite);
    gtk_widget_show(parasite->action_list);
    gtk_container_add(GTK_CONTAINER(swin), parasite->action_list);

    return vbox;
}

void
gtkparasite_window_create()
{
    ParasiteWindow *window;
    GtkWidget *vpaned;
    GtkWidget *notebook;
    char *title;

    window = g_new0(ParasiteWindow, 1);

    /*
     * Create the top-level window.
     */
    window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window->window), 1000, 500);
    gtk_container_set_border_width(GTK_CONTAINER(window->window), 12);
    gtk_widget_show(window->window);

    title = g_strdup_printf("Parasite - %s", g_get_application_name());
    gtk_window_set_title(GTK_WINDOW(window->window), title);
    g_free(title);

    vpaned = gtk_vpaned_new();
    gtk_widget_show(vpaned);
    gtk_container_add(GTK_CONTAINER(window->window), vpaned);

    notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    gtk_paned_pack1(GTK_PANED(vpaned), notebook, TRUE, FALSE);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             create_widget_tree(window),
                             gtk_label_new("Widget Tree"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             create_action_list(window),
                             gtk_label_new("Action List"));
}

// vim: set et sw=4 ts=4:
