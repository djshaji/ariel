#include "ariel.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

// CLI-specific enums and structures
typedef enum {
    CLI_PANEL_PLUGIN_LIST = 0,
    CLI_PANEL_ACTIVE_PLUGINS = 1,
    CLI_PANEL_PLUGIN_CONTROLS = 2,
    CLI_PANEL_COUNT = 3
} ArielCLIPanelType;

typedef struct {
    WINDOW *plugin_list_win;
    WINDOW *active_plugins_win;
    WINDOW *plugin_controls_win;
    WINDOW *controls_win;
    WINDOW *status_win;
    
    ArielApp *app;
    ArielPluginManager *plugin_manager;
    ArielAudioEngine *audio_engine;
    
    int plugin_list_selected;
    int active_plugin_selected;
    int plugin_control_selected;
    int max_plugins;
    int max_active_plugins;
    int max_plugin_controls;
    
    gboolean running;
    gboolean audio_active;
    gboolean show_help;
    ArielCLIPanelType current_panel;
    
    // Scrolling state
    int plugin_list_scroll_offset;
    int active_plugins_scroll_offset;
    int plugin_controls_scroll_offset;
} ArielCLI;

static ArielCLI *g_cli = NULL;

// Helper structure for output suppression
typedef struct {
    int stdout_fd;
    int stderr_fd;
    int null_fd;
} OutputSuppressor;

// Helper functions for output suppression
static OutputSuppressor* suppress_output()
{
    OutputSuppressor *suppressor = g_malloc0(sizeof(OutputSuppressor));
    suppressor->stdout_fd = dup(STDOUT_FILENO);
    suppressor->stderr_fd = dup(STDERR_FILENO);
    suppressor->null_fd = open("/dev/null", O_WRONLY);
    
    if (suppressor->null_fd != -1) {
        dup2(suppressor->null_fd, STDOUT_FILENO);
        dup2(suppressor->null_fd, STDERR_FILENO);
        close(suppressor->null_fd);
    }
    
    return suppressor;
}

static void restore_output(OutputSuppressor *suppressor)
{
    if (!suppressor) return;
    
    if (suppressor->stdout_fd != -1) {
        dup2(suppressor->stdout_fd, STDOUT_FILENO);
        close(suppressor->stdout_fd);
    }
    if (suppressor->stderr_fd != -1) {
        dup2(suppressor->stderr_fd, STDERR_FILENO);
        close(suppressor->stderr_fd);
    }
    
    g_free(suppressor);
}

// Function prototypes
static void cli_init_windows(ArielCLI *cli);
static void cli_draw_header(ArielCLI *cli);
static void cli_draw_plugin_list(ArielCLI *cli);
static void cli_draw_active_plugins(ArielCLI *cli);
static void cli_draw_plugin_controls(ArielCLI *cli);
static void cli_draw_controls(ArielCLI *cli);
static void cli_draw_status(ArielCLI *cli);
static void cli_refresh_all(ArielCLI *cli);
static void cli_handle_input(ArielCLI *cli, int ch);
static void cli_add_plugin(ArielCLI *cli);
static void cli_remove_plugin(ArielCLI *cli);
static void cli_toggle_plugin(ArielCLI *cli);
static void cli_toggle_audio_engine(ArielCLI *cli);
static void cli_toggle_bypass(ArielCLI *cli);
static void cli_load_file(ArielCLI *cli);
static void cli_adjust_parameter(ArielCLI *cli, float delta);
static void cli_cleanup(ArielCLI *cli);

// Get plugin name safely
static const char *get_plugin_name(ArielPluginInfo *plugin_info)
{
    if (!plugin_info) return "Unknown Plugin";
    
    // Use the built-in function that already handles this
    const char *name = ariel_plugin_info_get_name(plugin_info);
    return name ? name : "Unnamed Plugin";
}

// Get active plugin name safely
static const char *get_active_plugin_name(ArielActivePlugin *active_plugin)
{
    if (!active_plugin) return "Unknown Active Plugin";
    
    // Try to get the plugin name from the active plugin
    ArielPluginInfo *plugin_info = ariel_active_plugin_get_plugin_info(active_plugin);
    if (plugin_info) {
        return get_plugin_name(plugin_info);
    }
    
    return "Active Plugin";
}

static void cli_init_windows(ArielCLI *cli)
{
    int height, width;
    getmaxyx(stdscr, height, width);
    
    // Ensure minimum terminal size
    if (height < 20 || width < 80) {
        endwin();
        fprintf(stderr, "Terminal too small. Need at least 80x20 characters.\n");
        exit(1);
    }
    
    // Delete old windows if they exist
    if (cli->plugin_list_win) delwin(cli->plugin_list_win);
    if (cli->active_plugins_win) delwin(cli->active_plugins_win);
    if (cli->plugin_controls_win) delwin(cli->plugin_controls_win);
    if (cli->controls_win) delwin(cli->controls_win);
    if (cli->status_win) delwin(cli->status_win);
    
    // Create windows with borders
    // Plugin list (left side, 30% width)
    int plugin_list_width = width * 0.3;
    cli->plugin_list_win = newwin(height - 4, plugin_list_width, 2, 0);
    
    // Right side divided into three panels
    int right_width = width - plugin_list_width;
    int panel_height = (height - 4) / 3;
    
    // Active plugins (top right)
    cli->active_plugins_win = newwin(panel_height, right_width, 2, plugin_list_width);
    
    // Plugin controls (middle right)
    cli->plugin_controls_win = newwin(panel_height, right_width, 2 + panel_height, plugin_list_width);
    
    // Help/Controls (bottom right)
    int controls_height = height - 4 - (2 * panel_height);
    cli->controls_win = newwin(controls_height, right_width, 2 + (2 * panel_height), plugin_list_width);
    
    // Status bar (bottom)
    cli->status_win = newwin(2, width, height - 2, 0);
    
    // Enable keypad for all windows
    keypad(cli->plugin_list_win, TRUE);
    keypad(cli->active_plugins_win, TRUE);
    keypad(cli->controls_win, TRUE);
    keypad(stdscr, TRUE);
    
    // Just clear the windows without refreshing them yet
    wclear(cli->plugin_list_win);
    wclear(cli->active_plugins_win);
    wclear(cli->plugin_controls_win);
    wclear(cli->controls_win);
    wclear(cli->status_win);
}

static void cli_draw_header(ArielCLI *cli)
{
    int height, width;
    getmaxyx(stdscr, height, width);
    
    // Draw title
    attron(A_BOLD | A_REVERSE);
    mvprintw(0, (width - strlen("Ariel LV2 Host - CLI Mode")) / 2, "Ariel LV2 Host - CLI Mode");
    attroff(A_BOLD | A_REVERSE);
    
    // Draw audio engine status
    if (cli->audio_active) {
        attron(COLOR_PAIR(2)); // Green
        mvprintw(1, 2, "[AUDIO: ON]");
        attroff(COLOR_PAIR(2));
    } else {
        attron(COLOR_PAIR(3)); // Red
        mvprintw(1, 2, "[AUDIO: OFF]");
        attroff(COLOR_PAIR(3));
    }
    
    // Draw sample rate and buffer size if audio is active
    if (cli->audio_active && cli->audio_engine) {
        mvprintw(1, 16, "SR: %.0f Hz  BS: %d", 
                cli->audio_engine->sample_rate, 
                cli->audio_engine->buffer_size);
    }
    
    // Draw help text
    mvprintw(1, width - 30, "Press 'h' for help");
}

static void cli_draw_plugin_list(ArielCLI *cli)
{
    if (!cli->plugin_manager || !cli->plugin_list_win) return;
    
    // Clear window content
    werase(cli->plugin_list_win);
    box(cli->plugin_list_win, 0, 0);
    
    // Window title with focus indicator
    const char *title = (cli->current_panel == CLI_PANEL_PLUGIN_LIST) ? 
                       " Available Plugins [FOCUSED] " : " Available Plugins ";
    if (cli->current_panel == CLI_PANEL_PLUGIN_LIST) {
        wattron(cli->plugin_list_win, A_BOLD | COLOR_PAIR(5));
    }
    mvwprintw(cli->plugin_list_win, 0, 2, "%s", title);
    if (cli->current_panel == CLI_PANEL_PLUGIN_LIST) {
        wattroff(cli->plugin_list_win, A_BOLD | COLOR_PAIR(5));
    }
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->plugin_store);
    guint n_plugins = g_list_model_get_n_items(model);
    cli->max_plugins = n_plugins;
    
    int win_height, win_width;
    getmaxyx(cli->plugin_list_win, win_height, win_width);
    
    if (n_plugins == 0) {
        mvwprintw(cli->plugin_list_win, 2, 2, "No plugins available");
        mvwprintw(cli->plugin_list_win, 3, 2, "Press 'r' to refresh");
    } else {
        int display_height = win_height - 3; // Account for borders and scrolling info
        
        // Calculate scroll offset to keep selected item visible
        if (cli->plugin_list_selected < cli->plugin_list_scroll_offset) {
            cli->plugin_list_scroll_offset = cli->plugin_list_selected;
        } else if (cli->plugin_list_selected >= cli->plugin_list_scroll_offset + display_height) {
            cli->plugin_list_scroll_offset = cli->plugin_list_selected - display_height + 1;
        }
        
        int start_idx = cli->plugin_list_scroll_offset;
        
        for (int i = 0; i < display_height && (start_idx + i) < n_plugins; i++) {
        ArielPluginInfo *plugin_info = g_list_model_get_item(model, start_idx + i);
        if (!plugin_info) continue;
        
        const char *name = get_plugin_name(plugin_info);
        
        // Highlight selected item
        if ((start_idx + i) == cli->plugin_list_selected) {
            wattron(cli->plugin_list_win, A_REVERSE);
        }
        
        // Truncate name if too long
        char display_name[win_width - 4];
        strncpy(display_name, name, sizeof(display_name) - 1);
        display_name[sizeof(display_name) - 1] = '\0';
        if (strlen(display_name) >= win_width - 4) {
            display_name[win_width - 7] = '.';
            display_name[win_width - 6] = '.';
            display_name[win_width - 5] = '.';
            display_name[win_width - 4] = '\0';
        }
        
        mvwprintw(cli->plugin_list_win, i + 1, 2, "%s", display_name);
        
        if ((start_idx + i) == cli->plugin_list_selected) {
            wattroff(cli->plugin_list_win, A_REVERSE);
        }
        
            g_object_unref(plugin_info);
        }
        
        // Show scrolling indicator
        if (n_plugins > display_height) {
            mvwprintw(cli->plugin_list_win, win_height - 1, win_width - 10, " %d/%d ", 
                     cli->plugin_list_selected + 1, n_plugins);
        }
    }
    
    wrefresh(cli->plugin_list_win);
}

static void cli_draw_active_plugins(ArielCLI *cli)
{
    if (!cli->plugin_manager || !cli->active_plugins_win) return;
    
    // Clear window content
    werase(cli->active_plugins_win);
    box(cli->active_plugins_win, 0, 0);
    
    // Window title with focus indicator
    const char *title = (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) ? 
                       " Active Plugins [FOCUSED] " : " Active Plugins ";
    if (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) {
        wattron(cli->active_plugins_win, A_BOLD | COLOR_PAIR(5));
    }
    mvwprintw(cli->active_plugins_win, 0, 2, "%s", title);
    if (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) {
        wattroff(cli->active_plugins_win, A_BOLD | COLOR_PAIR(5));
    }
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->active_plugin_store);
    guint n_active = g_list_model_get_n_items(model);
    cli->max_active_plugins = n_active;
    
    int win_height, win_width;
    getmaxyx(cli->active_plugins_win, win_height, win_width);
    
    if (n_active == 0) {
        mvwprintw(cli->active_plugins_win, 2, 2, "No active plugins");
    } else {
        int display_height = win_height - 3; // Leave room for scrolling info
        
        // Calculate scroll offset to keep selected item visible
        if (cli->active_plugin_selected < cli->active_plugins_scroll_offset) {
            cli->active_plugins_scroll_offset = cli->active_plugin_selected;
        } else if (cli->active_plugin_selected >= cli->active_plugins_scroll_offset + display_height) {
            cli->active_plugins_scroll_offset = cli->active_plugin_selected - display_height + 1;
        }
        
        int start_idx = cli->active_plugins_scroll_offset;
        
        for (int i = 0; i < display_height && (start_idx + i) < n_active; i++) {
            ArielActivePlugin *active_plugin = g_list_model_get_item(model, start_idx + i);
            if (!active_plugin) continue;
            
            const char *name = get_active_plugin_name(active_plugin);
            gboolean is_active = ariel_active_plugin_is_active(active_plugin);
            
            // Highlight selected item
            if ((start_idx + i) == cli->active_plugin_selected) {
                wattron(cli->active_plugins_win, A_REVERSE);
            }
            
            // Show status with color
            if (is_active) {
                wattron(cli->active_plugins_win, COLOR_PAIR(2)); // Green
                mvwprintw(cli->active_plugins_win, i + 1, 2, "[ON] ");
                wattroff(cli->active_plugins_win, COLOR_PAIR(2));
            } else {
                wattron(cli->active_plugins_win, COLOR_PAIR(3)); // Red
                mvwprintw(cli->active_plugins_win, i + 1, 2, "[OFF]");
                wattroff(cli->active_plugins_win, COLOR_PAIR(3));
            }
            
            // Plugin name
            char display_name[win_width - 8];
            strncpy(display_name, name, sizeof(display_name) - 1);
            display_name[sizeof(display_name) - 1] = '\0';
            if (strlen(display_name) >= win_width - 8) {
                display_name[win_width - 11] = '.';
                display_name[win_width - 10] = '.';
                display_name[win_width - 9] = '.';
                display_name[win_width - 8] = '\0';
            }
            
            mvwprintw(cli->active_plugins_win, i + 1, 7, "%s", display_name);
            
            if ((start_idx + i) == cli->active_plugin_selected) {
                wattroff(cli->active_plugins_win, A_REVERSE);
            }
            
            g_object_unref(active_plugin);
        }
        
        // Show scrolling indicator
        if (n_active > display_height) {
            mvwprintw(cli->active_plugins_win, win_height - 1, win_width - 10, " %d/%d ", 
                     cli->active_plugin_selected + 1, n_active);
        }
    }
    
    wrefresh(cli->active_plugins_win);
}

static void cli_draw_plugin_controls(ArielCLI *cli)
{
    if (!cli->plugin_manager || !cli->plugin_controls_win) return;
    
    // Clear window content
    werase(cli->plugin_controls_win);
    box(cli->plugin_controls_win, 0, 0);
    
    // Window title with focus indicator
    const char *title = (cli->current_panel == CLI_PANEL_PLUGIN_CONTROLS) ? 
                       " Plugin Controls [FOCUSED] " : " Plugin Controls ";
    if (cli->current_panel == CLI_PANEL_PLUGIN_CONTROLS) {
        wattron(cli->plugin_controls_win, A_BOLD | COLOR_PAIR(5));
    }
    mvwprintw(cli->plugin_controls_win, 0, 2, "%s", title);
    if (cli->current_panel == CLI_PANEL_PLUGIN_CONTROLS) {
        wattroff(cli->plugin_controls_win, A_BOLD | COLOR_PAIR(5));
    }
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->active_plugin_store);
    guint n_active = g_list_model_get_n_items(model);
    
    int win_height, win_width;
    getmaxyx(cli->plugin_controls_win, win_height, win_width);
    
    if (n_active == 0 || cli->active_plugin_selected >= n_active) {
        mvwprintw(cli->plugin_controls_win, 2, 2, "No active plugin selected");
        mvwprintw(cli->plugin_controls_win, 3, 2, "Select a plugin from the active list");
    } else {
        // Get the currently selected active plugin
        ArielActivePlugin *plugin = g_list_model_get_item(model, cli->active_plugin_selected);
        if (!plugin) {
            mvwprintw(cli->plugin_controls_win, 2, 2, "Plugin not available");
            wrefresh(cli->plugin_controls_win);
            return;
        }
        
        const char *plugin_name = get_active_plugin_name(plugin);
        mvwprintw(cli->plugin_controls_win, 1, 2, "Controls for: %.*s", 
                 win_width - 18, plugin_name);
        
        // Get plugin parameters
        uint32_t n_params = ariel_active_plugin_get_num_parameters(plugin);
        cli->max_plugin_controls = n_params + 2; // +2 for bypass and file loading
        
        int display_height = win_height - 4; // Account for borders and title
        
        // Calculate scroll offset
        if (cli->plugin_control_selected < cli->plugin_controls_scroll_offset) {
            cli->plugin_controls_scroll_offset = cli->plugin_control_selected;
        } else if (cli->plugin_control_selected >= cli->plugin_controls_scroll_offset + display_height) {
            cli->plugin_controls_scroll_offset = cli->plugin_control_selected - display_height + 1;
        }
        
        int start_idx = cli->plugin_controls_scroll_offset;
        int row = 3;
        
        // Show bypass control first
        if (start_idx <= 0 && row < win_height - 1) {
            gboolean bypassed = ariel_active_plugin_get_bypass(plugin);
            if (cli->plugin_control_selected == 0) {
                wattron(cli->plugin_controls_win, A_REVERSE);
            }
            if (bypassed) {
                wattron(cli->plugin_controls_win, COLOR_PAIR(3)); // Red
                mvwprintw(cli->plugin_controls_win, row, 2, "[BYPASS] ON ");
                wattroff(cli->plugin_controls_win, COLOR_PAIR(3));
            } else {
                wattron(cli->plugin_controls_win, COLOR_PAIR(2)); // Green
                mvwprintw(cli->plugin_controls_win, row, 2, "[BYPASS] OFF");
                wattroff(cli->plugin_controls_win, COLOR_PAIR(2));
            }
            mvwprintw(cli->plugin_controls_win, row, 15, " (Press 'b' to toggle)");
            if (cli->plugin_control_selected == 0) {
                wattroff(cli->plugin_controls_win, A_REVERSE);
            }
            row++;
        }
        
        // Show file loading control if supported
        if (start_idx <= 1 && row < win_height - 1) {
            gboolean supports_files = ariel_active_plugin_supports_file_parameters(plugin);
            if (cli->plugin_control_selected == 1) {
                wattron(cli->plugin_controls_win, A_REVERSE);
            }
            if (supports_files) {
                wattron(cli->plugin_controls_win, COLOR_PAIR(2)); // Green
                mvwprintw(cli->plugin_controls_win, row, 2, "[FILE LOAD] Available");
                wattroff(cli->plugin_controls_win, COLOR_PAIR(2));
                mvwprintw(cli->plugin_controls_win, row, 22, " (Press 'f' to load)");
            } else {
                wattron(cli->plugin_controls_win, COLOR_PAIR(4)); // Yellow
                mvwprintw(cli->plugin_controls_win, row, 2, "[FILE LOAD] Not supported");
                wattroff(cli->plugin_controls_win, COLOR_PAIR(4));
            }
            if (cli->plugin_control_selected == 1) {
                wattroff(cli->plugin_controls_win, A_REVERSE);
            }
            row++;
        }
        
        // Show parameters
        for (uint32_t i = 0; i < n_params && row < win_height - 1; i++) {
            int control_idx = i + 2; // +2 for bypass and file loading
            if (control_idx < start_idx) continue;
            if (control_idx >= start_idx + display_height) break;
            
            float value = ariel_active_plugin_get_parameter(plugin, i);
            
            if (cli->plugin_control_selected == control_idx) {
                wattron(cli->plugin_controls_win, A_REVERSE);
            }
            
            // Show parameter name and value
            mvwprintw(cli->plugin_controls_win, row, 2, "Param %d: %.3f", i, value);
            mvwprintw(cli->plugin_controls_win, row, 20, " (+/- to adjust)");
            
            if (cli->plugin_control_selected == control_idx) {
                wattroff(cli->plugin_controls_win, A_REVERSE);
            }
            row++;
        }
        
        // Show scrolling indicator
        if (cli->max_plugin_controls > display_height) {
            mvwprintw(cli->plugin_controls_win, win_height - 1, win_width - 10, " %d/%d ", 
                     cli->plugin_control_selected + 1, cli->max_plugin_controls);
        }
        
        g_object_unref(plugin);
    }
    
    wrefresh(cli->plugin_controls_win);
}

static void cli_draw_controls(ArielCLI *cli)
{
    if (!cli->controls_win) return;
    
    // Clear window content
    werase(cli->controls_win);
    
    if (!cli->show_help) {
        // Show minimized help hint
        mvwprintw(cli->controls_win, 0, 2, " Press 'h' for help ");
        wrefresh(cli->controls_win);
        return;
    }
    
    box(cli->controls_win, 0, 0);
    
    // Window title
    mvwprintw(cli->controls_win, 0, 2, " Controls ");
    
    int row = 2;
    
    // Plugin controls
    mvwprintw(cli->controls_win, row++, 2, "Plugin Controls:");
    mvwprintw(cli->controls_win, row++, 4, "a - Add selected plugin");
    mvwprintw(cli->controls_win, row++, 4, "d - Remove active plugin");
    mvwprintw(cli->controls_win, row++, 4, "t - Toggle active plugin");
    mvwprintw(cli->controls_win, row++, 4, "b - Toggle bypass (in controls)");
    mvwprintw(cli->controls_win, row++, 4, "f - Load file (in controls)");
    mvwprintw(cli->controls_win, row++, 4, "+/- - Adjust parameter (controls)");
    row++;
    
    // Audio controls
    mvwprintw(cli->controls_win, row++, 2, "Audio Controls:");
    mvwprintw(cli->controls_win, row++, 4, "s - Start/Stop audio engine");
    row++;
    
    // Navigation
    mvwprintw(cli->controls_win, row++, 2, "Navigation:");
    mvwprintw(cli->controls_win, row++, 4, "Tab/←→ - Switch panels");
    mvwprintw(cli->controls_win, row++, 4, "↑↓ - Navigate lists");
    mvwprintw(cli->controls_win, row++, 4, "PgUp/PgDn - Page navigation");
    mvwprintw(cli->controls_win, row++, 4, "Home/End - Jump to start/end");
    row++;
    
    // General controls
    mvwprintw(cli->controls_win, row++, 2, "General:");
    mvwprintw(cli->controls_win, row++, 4, "r - Refresh plugin list");
    mvwprintw(cli->controls_win, row++, 4, "Ctrl+L - Force screen refresh");
    mvwprintw(cli->controls_win, row++, 4, "h - Show/hide help");
    mvwprintw(cli->controls_win, row++, 4, "q - Quit application");
    
    wrefresh(cli->controls_win);
}

static void cli_draw_status(ArielCLI *cli)
{
    if (!cli->status_win) return;
    
    // Clear status window
    werase(cli->status_win);
    
    int width;
    int height;
    getmaxyx(cli->status_win, height, width);
    
    // Status line
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->plugin_store);
    guint n_plugins = g_list_model_get_n_items(model);
    
    GListModel *active_model = G_LIST_MODEL(cli->plugin_manager->active_plugin_store);
    guint n_active = g_list_model_get_n_items(active_model);
    
    // Audio engine status
    const char *audio_status = cli->audio_active ? "ON" : "OFF";
    if (cli->audio_active) {
        wattron(cli->status_win, COLOR_PAIR(2)); // Green
    } else {
        wattron(cli->status_win, COLOR_PAIR(3)); // Red
    }
    mvwprintw(cli->status_win, 0, 2, "Audio: %s", audio_status);
    if (cli->audio_active) {
        wattroff(cli->status_win, COLOR_PAIR(2));
    } else {
        wattroff(cli->status_win, COLOR_PAIR(3));
    }
    
    mvwprintw(cli->status_win, 0, 15, "| Plugins: %d | Active: %d", n_plugins, n_active);
    
    // Current selection info
    if (cli->max_plugins > 0) {
        mvwprintw(cli->status_win, 1, 2, "Selected: %d/%d", 
                 cli->plugin_list_selected + 1, cli->max_plugins);
    }
    
    // Show current time (only minutes and hours to reduce updates)
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%H:%M", tm_info);
    mvwprintw(cli->status_win, 0, width - 10, "%s", time_str);
    
    wrefresh(cli->status_win);
}

static void cli_refresh_all(ArielCLI *cli)
{
    // Only clear the header area, not the entire screen
    move(0, 0);
    clrtoeol();
    move(1, 0);
    clrtoeol();
    
    // Draw header directly on stdscr
    cli_draw_header(cli);
    
    // Draw all windows - they handle their own clearing
    cli_draw_plugin_list(cli);
    cli_draw_active_plugins(cli);
    cli_draw_plugin_controls(cli);
    cli_draw_controls(cli);
    cli_draw_status(cli);
    
    // Update everything at once for smooth display
    doupdate();
}

static void cli_handle_input(ArielCLI *cli, int ch)
{
    switch (ch) {
        case 'q':
        case 'Q':
            cli->running = FALSE;
            break;
            
        case 'a':
        case 'A':
            cli_add_plugin(cli);
            break;
            
        case 'd':
        case 'D':
            cli_remove_plugin(cli);
            break;
            
        case 't':
        case 'T':
            cli_toggle_plugin(cli);
            break;
            
        case 's':
        case 'S':
            cli_toggle_audio_engine(cli);
            break;
            
        case 'r':
        case 'R':
            if (cli->plugin_manager) {
                // Suppress output during plugin refresh
                OutputSuppressor *suppressor = suppress_output();
                ariel_plugin_manager_refresh(cli->plugin_manager);
                restore_output(suppressor);
            }
            break;
            
        case 12: // Ctrl+L - force refresh screen
            clear();
            cli_init_windows(cli);
            break;
            
        case 'b':
        case 'B':
            cli_toggle_bypass(cli);
            break;
            
        case 'f':
        case 'F':
            if (cli->current_panel == CLI_PANEL_PLUGIN_CONTROLS) {
                cli_load_file(cli);
            }
            break;
            
        case '+':
        case '=':
            if (cli->current_panel == CLI_PANEL_PLUGIN_CONTROLS) {
                cli_adjust_parameter(cli, 0.1f);
            }
            break;
            
        case '-':
        case '_':
            if (cli->current_panel == CLI_PANEL_PLUGIN_CONTROLS) {
                cli_adjust_parameter(cli, -0.1f);
            }
            break;
            
        case KEY_UP:
            if (cli->current_panel == CLI_PANEL_PLUGIN_LIST) {
                if (cli->plugin_list_selected > 0) {
                    cli->plugin_list_selected--;
                }
            } else if (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) {
                if (cli->active_plugin_selected > 0) {
                    cli->active_plugin_selected--;
                }
            } else if (cli->current_panel == CLI_PANEL_PLUGIN_CONTROLS) {
                if (cli->plugin_control_selected > 0) {
                    cli->plugin_control_selected--;
                }
            }
            break;
            
        case KEY_DOWN:
            if (cli->current_panel == CLI_PANEL_PLUGIN_LIST) {
                if (cli->plugin_list_selected < cli->max_plugins - 1) {
                    cli->plugin_list_selected++;
                }
            } else if (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) {
                if (cli->active_plugin_selected < cli->max_active_plugins - 1) {
                    cli->active_plugin_selected++;
                }
            } else if (cli->current_panel == CLI_PANEL_PLUGIN_CONTROLS) {
                if (cli->plugin_control_selected < cli->max_plugin_controls - 1) {
                    cli->plugin_control_selected++;
                }
            }
            break;
            
        case KEY_LEFT:
            // Switch to previous panel
            cli->current_panel = (cli->current_panel - 1 + CLI_PANEL_COUNT) % CLI_PANEL_COUNT;
            break;
            
        case KEY_RIGHT:
            // Switch to next panel
            cli->current_panel = (cli->current_panel + 1) % CLI_PANEL_COUNT;
            break;
            
        case '\t': // Tab key
            // Switch focus between panels
            cli->current_panel = (cli->current_panel + 1) % CLI_PANEL_COUNT;
            break;
            
        case 'h':
        case 'H':
            // Toggle help display (controls window)
            cli->show_help = !cli->show_help;
            break;
            
        case KEY_NPAGE: // Page Down
            if (cli->current_panel == CLI_PANEL_PLUGIN_LIST) {
                int new_pos = cli->plugin_list_selected + 10;
                if (new_pos >= cli->max_plugins) {
                    new_pos = cli->max_plugins - 1;
                }
                if (new_pos >= 0) {
                    cli->plugin_list_selected = new_pos;
                }
            } else if (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) {
                int new_pos = cli->active_plugin_selected + 10;
                if (new_pos >= cli->max_active_plugins) {
                    new_pos = cli->max_active_plugins - 1;
                }
                if (new_pos >= 0) {
                    cli->active_plugin_selected = new_pos;
                }
            }
            break;
            
        case KEY_PPAGE: // Page Up
            if (cli->current_panel == CLI_PANEL_PLUGIN_LIST) {
                int new_pos = cli->plugin_list_selected - 10;
                if (new_pos < 0) {
                    new_pos = 0;
                }
                cli->plugin_list_selected = new_pos;
            } else if (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) {
                int new_pos = cli->active_plugin_selected - 10;
                if (new_pos < 0) {
                    new_pos = 0;
                }
                cli->active_plugin_selected = new_pos;
            }
            break;
            
        case KEY_HOME:
            if (cli->current_panel == CLI_PANEL_PLUGIN_LIST) {
                cli->plugin_list_selected = 0;
            } else if (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) {
                cli->active_plugin_selected = 0;
            }
            break;
            
        case KEY_END:
            if (cli->current_panel == CLI_PANEL_PLUGIN_LIST) {
                if (cli->max_plugins > 0) {
                    cli->plugin_list_selected = cli->max_plugins - 1;
                }
            } else if (cli->current_panel == CLI_PANEL_ACTIVE_PLUGINS) {
                if (cli->max_active_plugins > 0) {
                    cli->active_plugin_selected = cli->max_active_plugins - 1;
                }
            }
            break;
            
        case KEY_RESIZE:
            // Handle terminal resize
            endwin();
            refresh();
            cli_init_windows(cli);
            break;
    }
}

static void cli_add_plugin(ArielCLI *cli)
{
    if (!cli->plugin_manager || cli->plugin_list_selected < 0) return;
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->plugin_store);
    guint n_plugins = g_list_model_get_n_items(model);
    if (n_plugins == 0 || cli->plugin_list_selected >= n_plugins) return;
    
    ArielPluginInfo *plugin_info = g_list_model_get_item(model, cli->plugin_list_selected);
    if (!plugin_info) return;
    
    // Suppress output during plugin loading
    OutputSuppressor *suppressor = suppress_output();
    
    // Load the plugin
    ArielActivePlugin *active_plugin = ariel_plugin_manager_load_plugin(
        cli->plugin_manager, plugin_info, cli->audio_engine);
    
    // Restore output
    restore_output(suppressor);
    
    if (active_plugin) {
        // Plugin loaded successfully - activate it by default
        ariel_active_plugin_set_active(active_plugin, TRUE);
        g_object_unref(active_plugin);
    }
    
    g_object_unref(plugin_info);
}

static void cli_remove_plugin(ArielCLI *cli)
{
    if (!cli->plugin_manager || cli->active_plugin_selected < 0) return;
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->active_plugin_store);
    guint n_active = g_list_model_get_n_items(model);
    if (n_active == 0 || cli->active_plugin_selected >= n_active) return;
    
    // Suppress output during plugin operations
    OutputSuppressor *suppressor = suppress_output();
    
    // Get the plugin before removing it to properly clean up
    ArielActivePlugin *active_plugin = g_list_model_get_item(model, cli->active_plugin_selected);
    if (active_plugin) {
        // Deactivate before removing
        ariel_active_plugin_set_active(active_plugin, FALSE);
        g_object_unref(active_plugin);
    }
    
    // Remove plugin from active list
    g_list_store_remove(cli->plugin_manager->active_plugin_store, cli->active_plugin_selected);
    
    // Restore output
    restore_output(suppressor);
    
    // Adjust selection if needed
    n_active = g_list_model_get_n_items(model); // Refresh count after removal
    if (cli->active_plugin_selected >= n_active && n_active > 0) {
        cli->active_plugin_selected = n_active - 1;
    } else if (n_active == 0) {
        cli->active_plugin_selected = 0;
    }
}

static void cli_toggle_plugin(ArielCLI *cli)
{
    if (!cli->plugin_manager || cli->active_plugin_selected < 0) return;
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->active_plugin_store);
    guint n_active = g_list_model_get_n_items(model);
    if (n_active == 0 || cli->active_plugin_selected >= n_active) return;
    
    ArielActivePlugin *active_plugin = g_list_model_get_item(model, cli->active_plugin_selected);
    if (!active_plugin) return;
    
    // Suppress output during plugin operations
    OutputSuppressor *suppressor = suppress_output();
    
    // Toggle plugin active state
    gboolean is_active = ariel_active_plugin_is_active(active_plugin);
    ariel_active_plugin_set_active(active_plugin, !is_active);
    
    // Restore output
    restore_output(suppressor);
    
    g_object_unref(active_plugin);
}

static void cli_toggle_audio_engine(ArielCLI *cli)
{
    if (!cli->audio_engine) return;
    
    // Suppress output during audio engine operations
    OutputSuppressor *suppressor = suppress_output();
    
    if (cli->audio_active) {
        ariel_audio_engine_stop(cli->audio_engine);
        cli->audio_active = FALSE;
    } else {
        cli->audio_active = ariel_audio_engine_start(cli->audio_engine);
    }
    
    // Restore output
    restore_output(suppressor);
}

static void cli_toggle_bypass(ArielCLI *cli)
{
    if (!cli->plugin_manager || cli->active_plugin_selected < 0) return;
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->active_plugin_store);
    guint n_active = g_list_model_get_n_items(model);
    if (n_active == 0 || cli->active_plugin_selected >= n_active) return;
    
    ArielActivePlugin *plugin = g_list_model_get_item(model, cli->active_plugin_selected);
    if (!plugin) return;
    
    // Suppress output during plugin operations
    OutputSuppressor *suppressor = suppress_output();
    
    // Toggle bypass state
    gboolean bypassed = ariel_active_plugin_get_bypass(plugin);
    ariel_active_plugin_set_bypass(plugin, !bypassed);
    
    // Restore output
    restore_output(suppressor);
    
    g_object_unref(plugin);
}

static void cli_load_file(ArielCLI *cli)
{
    if (!cli->plugin_manager || cli->active_plugin_selected < 0) return;
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->active_plugin_store);
    guint n_active = g_list_model_get_n_items(model);
    if (n_active == 0 || cli->active_plugin_selected >= n_active) return;
    
    ArielActivePlugin *plugin = g_list_model_get_item(model, cli->active_plugin_selected);
    if (!plugin) return;
    
    // Check if plugin supports file parameters
    if (!ariel_active_plugin_supports_file_parameters(plugin)) {
        g_object_unref(plugin);
        return;
    }
    
    // For now, we'll use a simple hardcoded path - in a full implementation,
    // this would show a file picker or prompt for input
    const char *test_file = "/tmp/test.wav"; // Placeholder
    
    // Suppress output during plugin operations
    OutputSuppressor *suppressor = suppress_output();
    
    // Load file parameter
    ariel_active_plugin_set_file_parameter(plugin, test_file);
    
    // Restore output
    restore_output(suppressor);
    
    g_object_unref(plugin);
}

static void cli_adjust_parameter(ArielCLI *cli, float delta)
{
    if (!cli->plugin_manager || cli->active_plugin_selected < 0) return;
    if (cli->current_panel != CLI_PANEL_PLUGIN_CONTROLS) return;
    
    GListModel *model = G_LIST_MODEL(cli->plugin_manager->active_plugin_store);
    guint n_active = g_list_model_get_n_items(model);
    if (n_active == 0 || cli->active_plugin_selected >= n_active) return;
    
    ArielActivePlugin *plugin = g_list_model_get_item(model, cli->active_plugin_selected);
    if (!plugin) return;
    
    // Skip bypass and file loading controls (indices 0 and 1)
    if (cli->plugin_control_selected < 2) {
        g_object_unref(plugin);
        return;
    }
    
    uint32_t param_idx = cli->plugin_control_selected - 2;
    uint32_t n_params = ariel_active_plugin_get_num_parameters(plugin);
    
    if (param_idx >= n_params) {
        g_object_unref(plugin);
        return;
    }
    
    // Suppress output during plugin operations
    OutputSuppressor *suppressor = suppress_output();
    
    // Get current value and adjust
    float current_value = ariel_active_plugin_get_parameter(plugin, param_idx);
    float new_value = current_value + delta;
    
    // Clamp to 0.0 - 1.0 range (most LV2 parameters are normalized)
    if (new_value < 0.0f) new_value = 0.0f;
    if (new_value > 1.0f) new_value = 1.0f;
    
    // Set the new parameter value
    ariel_active_plugin_set_parameter(plugin, param_idx, new_value);
    
    // Restore output
    restore_output(suppressor);
    
    g_object_unref(plugin);
}

static void cli_cleanup(ArielCLI *cli)
{
    if (!cli) return;
    
    // Cleanup windows
    if (cli->plugin_list_win) delwin(cli->plugin_list_win);
    if (cli->active_plugins_win) delwin(cli->active_plugins_win);
    if (cli->plugin_controls_win) delwin(cli->plugin_controls_win);
    if (cli->controls_win) delwin(cli->controls_win);
    if (cli->status_win) delwin(cli->status_win);
    
    // Stop audio engine
    if (cli->audio_engine && cli->audio_active) {
        ariel_audio_engine_stop(cli->audio_engine);
    }
    
    endwin();
}

int ariel_cli_main(int argc, char **argv)
{
    // GLib type system is automatically initialized in newer versions
    
    // Create CLI structure
    g_cli = g_malloc0(sizeof(ArielCLI));
    if (!g_cli) {
        g_critical("Failed to allocate CLI structure");
        return 1;
    }
    
    // Initialize Ariel components
    g_cli->app = ariel_app_new();
    if (!g_cli->app) {
        g_critical("Failed to create Ariel application");
        g_free(g_cli);
        return 1;
    }
    
    g_cli->plugin_manager = ariel_app_get_plugin_manager(g_cli->app);
    g_cli->audio_engine = ariel_app_get_audio_engine(g_cli->app);
    
    if (!g_cli->plugin_manager || !g_cli->audio_engine) {
        g_critical("Failed to get plugin manager or audio engine");
        g_object_unref(g_cli->app);
        g_free(g_cli);
        return 1;
    }
    
    // Set up plugin manager with audio engine
    ariel_audio_engine_set_plugin_manager(g_cli->audio_engine, g_cli->plugin_manager);
    
    // Initialize ncurses
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0); // Hide cursor
    // Remove timeout - we want blocking input to prevent constant refreshing
    
    // Initialize colors
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK);   // Normal
        init_pair(2, COLOR_GREEN, COLOR_BLACK);   // Active/On
        init_pair(3, COLOR_RED, COLOR_BLACK);     // Inactive/Off
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);  // Warning
        init_pair(5, COLOR_BLUE, COLOR_BLACK);    // Info
    }
    
    // Initialize CLI state
    g_cli->running = TRUE;
    g_cli->audio_active = FALSE;
    g_cli->show_help = TRUE; // Show help by default
    g_cli->current_panel = CLI_PANEL_PLUGIN_LIST;
    g_cli->plugin_list_selected = 0;
    g_cli->active_plugin_selected = 0;
    g_cli->plugin_control_selected = 0;
    g_cli->plugin_list_scroll_offset = 0;
    g_cli->active_plugins_scroll_offset = 0;
    g_cli->plugin_controls_scroll_offset = 0;
    
    // Create windows
    cli_init_windows(g_cli);
    
    // Load plugins
    if (g_cli->plugin_manager) {
        ariel_plugin_manager_refresh(g_cli->plugin_manager);
    }
    
    // Clear screen and ensure proper initial display
    clear();
    
    // Initial display - draw everything
    cli_refresh_all(g_cli);
    
    // Force initial screen update and display
    refresh();
    doupdate();
    
    // Main loop
    while (g_cli->running) {
        // Handle input - blocking call, no timeout
        int ch = getch();
        if (ch != ERR) {
            cli_handle_input(g_cli, ch);
            // Only refresh after user input
            cli_refresh_all(g_cli);
        }
    }
    
    // Cleanup
    cli_cleanup(g_cli);
    
    // Cleanup Ariel components
    g_object_unref(g_cli->app);
    g_free(g_cli);
    
    return 0;
}

// Check if CLI mode should be used
gboolean ariel_should_use_cli(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cli") == 0 || strcmp(argv[i], "-c") == 0) {
            return TRUE;
        }
    }
    return FALSE;
}