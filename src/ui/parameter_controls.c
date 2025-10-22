#include "ariel.h"
#include <math.h>

// Parameter control callback data
typedef struct {
    ArielActivePlugin *plugin;
    uint32_t param_index;
    GtkWidget *control_widget; // Reference to the control widget
    char *parameter_uri; // For file parameters - the actual parameter URI
} ParameterControlData;

// Callback for parameter value changes (scales)
static void
on_parameter_changed(GtkRange *range, ParameterControlData *data)
{
    if (!data || !data->plugin) return;
    
    double value = gtk_range_get_value(range);
    ariel_active_plugin_set_parameter(data->plugin, data->param_index, (float)value);
    
    ariel_log(INFO, "Parameter %u changed to %.3f", data->param_index, value);
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
    
    ariel_log(INFO, "Toggle parameter %u changed to %s", data->param_index, active ? "ON" : "OFF");
}



// Forward declarations
static void on_file_dialog_open_finish(GObject *source, GAsyncResult *result, gpointer user_data);

// Callback for file chooser button
static void
on_file_button_clicked(GtkButton *button, ParameterControlData *data)
{
    if (!data || !data->plugin) return;
    
    // Create modern GTK4 file dialog
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Neural Amp Model");
    
    // Set up file filters for neural amp models
    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    
    // Neural model files filter
    GtkFileFilter *nam_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(nam_filter, "Neural Amp Models (*.nam, *.nammodel)");
    gtk_file_filter_add_pattern(nam_filter, "*.nam");
    gtk_file_filter_add_pattern(nam_filter, "*.nammodel");
    g_list_store_append(filters, nam_filter);
    g_object_unref(nam_filter);
    
    // All files filter as backup
    GtkFileFilter *all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Files (*.*)");
    gtk_file_filter_add_pattern(all_filter, "*");
    g_list_store_append(filters, all_filter);
    g_object_unref(all_filter);
    
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    gtk_file_dialog_set_default_filter(dialog, nam_filter);
    if (filters) g_object_unref(filters);
    
    // Set initial directory to user's home
    const char *home_dir = g_get_home_dir();
    if (home_dir) {
        GFile *home_file = g_file_new_for_path(home_dir);
        gtk_file_dialog_set_initial_folder(dialog, home_file);
        g_object_unref(home_file);
    }
    
    // Get the parent window for the dialog
    GtkRoot *root = gtk_widget_get_root(GTK_WIDGET(button));
    GtkWindow *parent_window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : NULL;
    
    // Show async file dialog
    gtk_file_dialog_open(dialog, parent_window, NULL, 
                        on_file_dialog_open_finish, data);
    
    if (dialog) g_object_unref(dialog);
}

// Handle file dialog completion
static void
on_file_dialog_open_finish(GObject *source, GAsyncResult *result, gpointer user_data)
{
    ParameterControlData *data = (ParameterControlData *)user_data;
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = NULL;
    
    if (!data) {
        g_warning("No callback data in file dialog");
        return;
    }
    
    if (!data->plugin) {
        g_warning("No plugin in callback data");
        return;
    }
    
    if (!data->control_widget) {
        g_warning("No control widget in callback data");
        return;
    }
    
    // Verify the plugin is still valid
    if (!G_IS_OBJECT(data->plugin)) {
        g_warning("Plugin object is invalid in file dialog callback");
        return;
    }
    
    // Get the selected file
    GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);
    
    if (file) {
        char *file_path = g_file_get_path(file);
        if (file_path) {
            ariel_log(INFO, "Selected neural model file: %s", file_path);
            
            // Validate file extension
            if (g_str_has_suffix(file_path, ".nam") || g_str_has_suffix(file_path, ".nammodel")) {
                // Double-check plugin validity before sending file parameter
                if (ariel_active_plugin_supports_file_parameters(data->plugin) && data->parameter_uri) {
                    ariel_log(INFO, "Sending file parameter to plugin: %s (URI: %s)", file_path, data->parameter_uri);
                    // Send file path to plugin via Atom message
                    ariel_active_plugin_set_file_parameter_with_uri(data->plugin, file_path, data->parameter_uri);
                } else {
                    g_warning("Plugin does not support file parameters or parameter URI is missing");
                }
                
                // Update button label to show filename
                char *basename = g_path_get_basename(file_path);
                char *label = g_strdup_printf("📁 %s", basename);
                gtk_button_set_label(GTK_BUTTON(data->control_widget), label);
                
                // Add tooltip with full path
                gtk_widget_set_tooltip_text(data->control_widget, file_path);
                
                g_free(basename);
                g_free(label);
                
                g_print("Neural model loaded: %s\n", file_path);
            } else {
                g_warning("Invalid file type selected: %s. Please select a .nam or .nammodel file.", file_path);
                
                // Show error dialog
                GtkRoot *root = gtk_widget_get_root(data->control_widget);
                GtkWindow *parent_window = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : NULL;
                GtkAlertDialog *alert = gtk_alert_dialog_new("Invalid File Type");
                gtk_alert_dialog_set_detail(alert, "Please select a Neural Amp Model file (.nam or .nammodel)");
                gtk_alert_dialog_show(alert, parent_window);
                if (alert) g_object_unref(alert);
            }
            
            g_free(file_path);
        }
        if (file) g_object_unref(file);
    } else if (error) {
        if (error->code != G_IO_ERROR_CANCELLED) {
            g_warning("File dialog error: %s", error->message);
        }
        g_error_free(error);
    }
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

// Check if port is an Atom control port with rdfs:range atom:Path
static gboolean
is_plugin_parameter_path(const LilvPlugin *plugin, const LilvPort *port)
{
    ArielApp *app = ARIEL_APP(g_application_get_default());
    ArielPluginManager *manager = ariel_app_get_plugin_manager(app);
    
    if (!manager || !manager->world || !port) return FALSE;
    
    // First check: is this an Atom port?
    LilvNode *atom_port_class = lilv_new_uri(manager->world, LV2_ATOM__AtomPort);
    LilvNode *input_port_class = lilv_new_uri(manager->world, LV2_CORE__InputPort);
    LilvNode *control_designation = lilv_new_uri(manager->world, LV2_CORE__control);
    
    gboolean is_atom_control_input = FALSE;
    
    // Check if it's an Atom input port with control designation
    if (lilv_port_is_a(plugin, port, atom_port_class) &&
        lilv_port_is_a(plugin, port, input_port_class)) {
        
        // Check if it has lv2:designation lv2:control
        LilvNodes *designations = lilv_port_get_value(plugin, port, 
                                   lilv_new_uri(manager->world, LV2_CORE__designation));
        if (designations) {
            LILV_FOREACH(nodes, i, designations) {
                const LilvNode *designation = lilv_nodes_get(designations, i);
                if (lilv_node_equals(designation, control_designation)) {
                    is_atom_control_input = TRUE;
                    break;
                }
            }
            lilv_nodes_free(designations);
        }
    }
    
    lilv_node_free(atom_port_class);
    lilv_node_free(input_port_class);
    lilv_node_free(control_designation);
    
    // If this is an Atom control input port, check if the plugin has
    // a parameter with atom:Path range that this port can handle
    if (is_atom_control_input) {
        const LilvNode *plugin_uri = lilv_plugin_get_uri(plugin);
        LilvNode *patch_writable = lilv_new_uri(manager->world, LV2_PATCH__writable);
        LilvNode *rdfs_range = lilv_new_uri(manager->world, "http://www.w3.org/2000/01/rdf-schema#range");
        LilvNode *atom_path = lilv_new_uri(manager->world, LV2_ATOM__Path);
        
        // Check if plugin has patch:writable parameters with atom:Path range
        LilvNodes *writables = lilv_world_find_nodes(manager->world, plugin_uri, patch_writable, NULL);
        if (writables) {
            LILV_FOREACH(nodes, i, writables) {
                const LilvNode *writable = lilv_nodes_get(writables, i);
                
                // Check if this parameter has rdfs:range atom:Path
                LilvNodes *ranges = lilv_world_find_nodes(manager->world, writable, rdfs_range, NULL);
                if (ranges) {
                    LILV_FOREACH(nodes, j, ranges) {
                        const LilvNode *range = lilv_nodes_get(ranges, j);
                        if (lilv_node_equals(range, atom_path)) {
                            lilv_nodes_free(ranges);
                            lilv_nodes_free(writables);
                            lilv_node_free(patch_writable);
                            lilv_node_free(rdfs_range);
                            lilv_node_free(atom_path);
                            return TRUE; // This Atom control port handles file parameters
                        }
                    }
                    lilv_nodes_free(ranges);
                }
            }
            lilv_nodes_free(writables);
        }
        
        lilv_node_free(patch_writable);
        lilv_node_free(rdfs_range);
        lilv_node_free(atom_path);
    }
    
    return FALSE;
}

// Create parameter control widget for a single parameter
static GtkWidget *
create_parameter_control(ArielActivePlugin *plugin, uint32_t param_index)
{
    const LilvPlugin *lilv_plugin = ariel_active_plugin_get_lilv_plugin(plugin);
    if (!lilv_plugin) return NULL;
    
    ariel_log (INFO, "Creating control for parameter index %u [%s]", param_index, 
               lilv_node_as_string(lilv_plugin_get_uri(lilv_plugin)));

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

// Create file parameter control for Atom control ports
static GtkWidget *
create_file_parameter_control(ArielActivePlugin *plugin, const LilvPlugin *lilv_plugin, const LilvPort *port)
{
    // Get parameter info
    char *label = get_parameter_label(lilv_plugin, port);
    ariel_log (INFO, "Creating file control for parameter [%s]", label);
    
    // Find the parameter URI for this atom:Path parameter
    char *parameter_uri = NULL;
    ArielApp *app = ARIEL_APP(g_application_get_default());
    ArielPluginManager *manager = ariel_app_get_plugin_manager(app);
    
    if (manager && manager->world) {
        const LilvNode *plugin_uri = lilv_plugin_get_uri(lilv_plugin);
        LilvNode *patch_writable = lilv_new_uri(manager->world, LV2_PATCH__writable);
        LilvNode *rdfs_range = lilv_new_uri(manager->world, "http://www.w3.org/2000/01/rdf-schema#range");
        LilvNode *atom_path = lilv_new_uri(manager->world, LV2_ATOM__Path);
        
        // Find the first parameter with atom:Path range
        LilvNodes *writables = lilv_world_find_nodes(manager->world, plugin_uri, patch_writable, NULL);
        if (writables) {
            LILV_FOREACH(nodes, i, writables) {
                const LilvNode *writable = lilv_nodes_get(writables, i);
                LilvNodes *ranges = lilv_world_find_nodes(manager->world, writable, rdfs_range, NULL);
                if (ranges) {
                    LILV_FOREACH(nodes, j, ranges) {
                        const LilvNode *range = lilv_nodes_get(ranges, j);
                        if (lilv_node_equals(range, atom_path)) {
                            parameter_uri = g_strdup(lilv_node_as_uri(writable));
                            lilv_nodes_free(ranges);
                            goto found_parameter;
                        }
                    }
                    lilv_nodes_free(ranges);
                }
            }
            found_parameter:
            lilv_nodes_free(writables);
        }
        
        lilv_node_free(patch_writable);
        lilv_node_free(rdfs_range);
        lilv_node_free(atom_path);
    }
    
    if (!parameter_uri) {
        g_warning("Could not find parameter URI for file parameter");
        g_free(label);
        return NULL;
    }
    
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
    data->param_index = 0; // This is for Atom messaging, not indexed parameters
    data->parameter_uri = parameter_uri; // Store the actual parameter URI
    
    // Create file chooser button for atom:Path parameters
    GtkWidget *control_widget = gtk_button_new_with_label("📁 Select Neural Model...");
    gtk_widget_add_css_class(control_widget, "pill");
    gtk_widget_add_css_class(control_widget, "suggested-action");
    
    data->control_widget = control_widget;
    
    // Add tooltip
    gtk_widget_set_tooltip_text(control_widget, "Click to select a Neural Amp Model file (.nam or .nammodel)");
    
    // File chooser callback for loading neural models  
    g_signal_connect_data(control_widget, "clicked",
                         G_CALLBACK(on_file_button_clicked), data,
                         NULL, 0);  // We'll handle cleanup in the callback
    
    gtk_box_append(GTK_BOX(param_box), control_widget);
    
    ariel_log(INFO, "Created file chooser button for Atom control port: %s", label);
    
    g_free(label);
    return param_box;
}

GtkWidget *
ariel_create_parameter_controls(ArielActivePlugin *plugin)
{
    if (!plugin) return NULL;
    
    const LilvPlugin *lilv_plugin = ariel_active_plugin_get_lilv_plugin(plugin);
    if (!lilv_plugin) return NULL;
    
    ariel_log(INFO, "Creating parameter controls for plugin: %s", 
              lilv_node_as_string(lilv_plugin_get_name(lilv_plugin)));
              
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
    
    // Get number of regular control parameters
    uint32_t num_params = ariel_active_plugin_get_num_parameters(plugin);
    
    // Check for Atom control ports with file parameters
    ArielApp *app = ARIEL_APP(g_application_get_default());
    ArielPluginManager *manager = ariel_app_get_plugin_manager(app);
    gboolean has_file_params = FALSE;
    
    if (manager && manager->world) {
        // Look for Atom control input ports
        for (uint32_t i = 0; i < lilv_plugin_get_num_ports(lilv_plugin); i++) {
            const LilvPort *port = lilv_plugin_get_port_by_index(lilv_plugin, i);
            if (is_plugin_parameter_path(lilv_plugin, port)) {
                has_file_params = TRUE;
                break;
            }
        }
    }
    
    if (num_params == 0 && !has_file_params) {
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
        
        // First, add Atom control ports with file parameters
        if (has_file_params) {
            for (uint32_t i = 0; i < lilv_plugin_get_num_ports(lilv_plugin); i++) {
                const LilvPort *port = lilv_plugin_get_port_by_index(lilv_plugin, i);
                if (is_plugin_parameter_path(lilv_plugin, port)) {
                    GtkWidget *file_control = create_file_parameter_control(plugin, lilv_plugin, port);
                    if (file_control) {
                        gtk_box_append(GTK_BOX(params_box), file_control);
                    }
                }
            }
            
            if (num_params > 0) {
                // Add separator between file params and regular params
                GtkWidget *separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
                gtk_box_append(GTK_BOX(params_box), separator2);
            }
        }
        
        // Then, add regular control parameters (Input Lvl, Output Lvl, etc.)
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