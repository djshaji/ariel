#include "ariel.h"
#include <math.h>

// Parameter control callback data
typedef struct {
    ArielActivePlugin *plugin;
    uint32_t param_index;
    GtkWidget *control_widget; // Reference to the control widget
} ParameterControlData;

// Callback for parameter value changes (scales)
static void
on_parameter_changed(GtkRange *range, ParameterControlData *data)
{
    if (!data || !data->plugin) return;
    
    double value = gtk_range_get_value(range);
    ariel_active_plugin_set_parameter(data->plugin, data->param_index, (float)value);
    
    g_print("Parameter %u changed to %.3f\n", data->param_index, value);
}

// Callback for toggle button changes
static void
on_toggle_changed(GtkToggleButton *button, ParameterControlData *data)
{
    if (!data || !data->plugin) return;
    
    gboolean active = gtk_toggle_button_get_active(button);
    float value = active ? 1.0f : 0.0f;
    ariel_active_plugin_set_parameter(data->plugin, data->param_index, value);
    
    // Update button label
    gtk_button_set_label(GTK_BUTTON(button), active ? "On" : "Off");
    
    g_print("Toggle parameter %u changed to %s\n", data->param_index, active ? "ON" : "OFF");
}

// Callback for file chooser button
static void
on_file_button_clicked(GtkButton *button, ParameterControlData *data)
{
    if (!data || !data->plugin) return;
    
    // Simple placeholder for now - just show that the button was clicked
    gtk_button_set_label(button, "📁 File Selected");
    ariel_active_plugin_set_parameter(data->plugin, data->param_index, 1.0f);
    g_print("File parameter %u: File selection triggered (neural model)\n", data->param_index);
}

// Get parameter label from lilv port
static char *
get_parameter_label(const LilvPlugin *plugin, const LilvPort *port)
{
    LilvNode *name_node = lilv_port_get_name(plugin, port);
    if (name_node) {
        char *label = g_strdup(lilv_node_as_string(name_node));
        lilv_node_free(name_node);
        return label;
    }
    return g_strdup("Parameter");
}

// Get parameter range from lilv port
static void
get_parameter_range(const LilvPlugin *plugin, const LilvPort *port, 
                   float *min_val, float *max_val, float *default_val)
{
    LilvNode *min_node = NULL, *max_node = NULL, *default_node = NULL;
    
    lilv_port_get_range(plugin, port, &default_node, &min_node, &max_node);
    
    if (min_node) {
        *min_val = lilv_node_as_float(min_node);
        lilv_node_free(min_node);
    } else {
        *min_val = 0.0f;
    }
    
    if (max_node) {
        *max_val = lilv_node_as_float(max_node);
        lilv_node_free(max_node);
    } else {
        *max_val = 1.0f;
    }
    
    if (default_node) {
        *default_val = lilv_node_as_float(default_node);
        lilv_node_free(default_node);
    } else {
        *default_val = (*min_val + *max_val) / 2.0f;
    }
}

// Check if port is a toggle (boolean) parameter
static gboolean
is_toggle_parameter(const LilvPlugin *plugin, const LilvPort *port)
{
    // Get plugin manager to access URID map
    ArielApp *app = ARIEL_APP(g_application_get_default());
    ArielPluginManager *manager = ariel_app_get_plugin_manager(app);
    
    if (!manager || !manager->world) return FALSE;
    
    LilvNode *toggled_uri = lilv_new_uri(manager->world, "http://lv2plug.in/ns/lv2core#toggled");
    gboolean is_toggle = lilv_port_has_property(plugin, port, toggled_uri);
    lilv_node_free(toggled_uri);
    
    return is_toggle;
}

// Check if port expects a file path (atom:Path)
static gboolean
is_path_parameter(const LilvPlugin *plugin, const LilvPort *port)
{
    // Get plugin manager to access URID map
    ArielApp *app = ARIEL_APP(g_application_get_default());
    ArielPluginManager *manager = ariel_app_get_plugin_manager(app);
    
    if (!manager || !manager->world) return FALSE;
    
    // Check if port expects atom:Path type directly
    LilvNode *path_uri = lilv_new_uri(manager->world, LV2_ATOM__Path);
    gboolean is_path = lilv_port_supports_event(plugin, port, path_uri);
    lilv_node_free(path_uri);
    
    return is_path;
}

// Check if plugin has LV2 Parameters with atom:Path range
static gboolean
is_plugin_parameter_path(const LilvPlugin *plugin, const LilvPort *port)
{
    ArielApp *app = ARIEL_APP(g_application_get_default());
    ArielPluginManager *manager = ariel_app_get_plugin_manager(app);
    
    if (!manager || !manager->world) return FALSE;
    
    // Get plugin URI to check for parameters
    const LilvNode *plugin_uri = lilv_plugin_get_uri(plugin);
    if (!plugin_uri) return FALSE;
    
    // Look for plugin parameters with atom:Path range
    LilvNode *param_uri = lilv_new_uri(manager->world, "http://lv2plug.in/ns/lv2core#Parameter");
    LilvNode *range_uri = lilv_new_uri(manager->world, "http://www.w3.org/2000/01/rdf-schema#range");
    LilvNode *path_uri = lilv_new_uri(manager->world, LV2_ATOM__Path);
    LilvNode *writable_uri = lilv_new_uri(manager->world, "http://lv2plug.in/ns/ext/patch#writable");
    
    // Check if plugin has patch:writable parameters with atom:Path range
    LilvNodes *writables = lilv_world_find_nodes(manager->world, plugin_uri, writable_uri, NULL);
    if (writables) {
        LILV_FOREACH(nodes, i, writables) {
            const LilvNode *writable = lilv_nodes_get(writables, i);
            
            // Check if this parameter has rdfs:range atom:Path
            LilvNodes *ranges = lilv_world_find_nodes(manager->world, writable, range_uri, NULL);
            if (ranges) {
                LILV_FOREACH(nodes, j, ranges) {
                    const LilvNode *range = lilv_nodes_get(ranges, j);
                    if (lilv_node_equals(range, path_uri)) {
                        // This parameter has atom:Path range - it's a file parameter
                        lilv_nodes_free(ranges);
                        lilv_nodes_free(writables);
                        lilv_node_free(param_uri);
                        lilv_node_free(range_uri);
                        lilv_node_free(path_uri);
                        lilv_node_free(writable_uri);
                        return TRUE;
                    }
                }
                lilv_nodes_free(ranges);
            }
        }
        lilv_nodes_free(writables);
    }
    
    lilv_node_free(param_uri);
    lilv_node_free(range_uri);
    lilv_node_free(path_uri);
    lilv_node_free(writable_uri);
    
    return FALSE;
}

// Create parameter control widget for a single parameter
static GtkWidget *
create_parameter_control(ArielActivePlugin *plugin, uint32_t param_index)
{
    const LilvPlugin *lilv_plugin = ariel_active_plugin_get_lilv_plugin(plugin);
    if (!lilv_plugin) return NULL;
    
    // Get the control input port for this parameter
    uint32_t control_port_index = ariel_active_plugin_get_control_port_index(plugin, param_index);
    const LilvPort *port = lilv_plugin_get_port_by_index(lilv_plugin, control_port_index);
    if (!port) return NULL;
    
    // Get parameter info
    char *label = get_parameter_label(lilv_plugin, port);
    float min_val, max_val, default_val;
    get_parameter_range(lilv_plugin, port, &min_val, &max_val, &default_val);
    
    // Create container box
    GtkWidget *param_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(param_box, 8);
    gtk_widget_set_margin_end(param_box, 8);
    gtk_widget_set_margin_top(param_box, 4);
    gtk_widget_set_margin_bottom(param_box, 4);
    
    // Create label
    GtkWidget *param_label = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(param_label), 0.0);
    gtk_widget_add_css_class(param_label, "caption");
    gtk_box_append(GTK_BOX(param_box), param_label);
    
    // Create callback data
    ParameterControlData *data = g_malloc(sizeof(ParameterControlData));
    data->plugin = plugin;
    data->param_index = param_index;
    
    GtkWidget *control_widget = NULL;
    
    // Determine control type based on port properties
    if (is_path_parameter(lilv_plugin, port) || is_plugin_parameter_path(lilv_plugin, port)) {
        // Create file chooser button for atom:Path parameters
        control_widget = gtk_button_new_with_label("📁 Select Neural Model...");
        gtk_widget_add_css_class(control_widget, "pill");
        gtk_widget_add_css_class(control_widget, "suggested-action");
        
        data->control_widget = control_widget;
        
        // File chooser callback for loading neural models
        g_signal_connect_data(control_widget, "clicked",
                             G_CALLBACK(on_file_button_clicked), data,
                             (GClosureNotify)g_free, 0);
        
        g_print("Created file chooser button for LV2 Parameter with atom:Path: %s\n", label);
        
    } else if (is_toggle_parameter(lilv_plugin, port)) {
        // Create toggle button for boolean parameters
        control_widget = gtk_toggle_button_new_with_label("Off");
        gtk_widget_add_css_class(control_widget, "pill");
        
        // Set initial state
        float current_value = ariel_active_plugin_get_parameter(plugin, param_index);
        gboolean is_active = (current_value > 0.5f);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control_widget), is_active);
        gtk_button_set_label(GTK_BUTTON(control_widget), is_active ? "On" : "Off");
        
        data->control_widget = control_widget;
        
        g_signal_connect_data(control_widget, "toggled",
                             G_CALLBACK(on_toggle_changed), data,
                             (GClosureNotify)g_free, 0);
        
        g_print("Created toggle button for parameter: %s\n", label);
        
    } else {
        // Create scale widget for continuous parameters
        control_widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
                                                 min_val, max_val, 
                                                 (max_val - min_val) / 100.0);
        gtk_scale_set_value_pos(GTK_SCALE(control_widget), GTK_POS_RIGHT);
        gtk_scale_set_digits(GTK_SCALE(control_widget), 2);
        gtk_range_set_value(GTK_RANGE(control_widget), default_val);
        
        // Set current parameter value



        float current_value = ariel_active_plugin_get_parameter(plugin, param_index);
        gtk_range_set_value(GTK_RANGE(control_widget), current_value);
        
        data->control_widget = control_widget;
        g_signal_connect_data(control_widget, "value-changed",
                             G_CALLBACK(on_parameter_changed), data,
                             (GClosureNotify)g_free, 0);
    }
    
    if (control_widget) {
        gtk_box_append(GTK_BOX(param_box), control_widget);
    }
    
    g_free(label);
    return param_box;
}

GtkWidget *
ariel_create_parameter_controls(ArielActivePlugin *plugin)
{
    if (!plugin) return NULL;
    
    // Create scrolled window for parameters
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 400);
    
    // Create container for all parameters
    GtkWidget *params_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(params_box, 12);
    gtk_widget_set_margin_end(params_box, 12);
    gtk_widget_set_margin_top(params_box, 12);
    gtk_widget_set_margin_bottom(params_box, 12);
    
    // Get number of parameters
    uint32_t num_params = ariel_active_plugin_get_num_parameters(plugin);
    
    if (num_params == 0) {
        // No parameters - show a message
        GtkWidget *no_params_label = gtk_label_new("This plugin has no adjustable parameters");
        gtk_label_set_wrap(GTK_LABEL(no_params_label), TRUE);
        gtk_widget_add_css_class(no_params_label, "dim-label");
        gtk_box_append(GTK_BOX(params_box), no_params_label);
    } else {
        // Create header
        GtkWidget *header_label = gtk_label_new("Plugin Parameters");
        gtk_widget_add_css_class(header_label, "title-4");
        gtk_label_set_xalign(GTK_LABEL(header_label), 0.0);
        gtk_box_append(GTK_BOX(params_box), header_label);
        
        // Create separator
        GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_append(GTK_BOX(params_box), separator);
        
        // Create parameter controls
        for (uint32_t i = 0; i < num_params; i++) {
            GtkWidget *param_control = create_parameter_control(plugin, i);
            if (param_control) {
                gtk_box_append(GTK_BOX(params_box), param_control);
            }
        }
    }
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), params_box);
    
    return scrolled;
}