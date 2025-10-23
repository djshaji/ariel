# Ariel Color Themes

This directory contains CSS color themes for the Ariel LV2 plugin host application. Each theme provides a different color scheme to customize the appearance of the application.

## Available Themes

### Primary Color Themes
- **teal.css** - Fresh teal/turquoise theme (#1abc9c)
- **violet.css** - Rich violet/purple theme (#8e44ad)
- **sunset.css** - Warm orange/sunset theme (#e67e22)
- **ocean.css** - Cool blue ocean theme (#3498db)
- **forest.css** - Natural green forest theme (#27ae60)
- **midnight.css** - Professional dark blue theme (#2c3e50)
- **rose.css** - Bold red/rose theme (#e74c3c)
- **gold.css** - Bright golden yellow theme (#f1c40f)
- **lavender.css** - Soft purple lavender theme (#9b59b6)
- **mint.css** - Fresh mint green theme (#2ecc71)

### Special Themes
- **soft-pink.css** - Gentle pink pastel theme (#ffb6c1)
- **dark.css** - Comprehensive dark theme for low-light environments

## How to Apply Themes

### Method 1: GTK CSS Loading (Recommended)
To apply a theme to Ariel, you can load the CSS file programmatically in your application:

```c
// In your main.c or window setup code
GtkCssProvider *css_provider = gtk_css_provider_new();
GdkDisplay *display = gdk_display_get_default();

// Load the theme CSS file
gtk_css_provider_load_from_path(css_provider, "themes/teal.css");

// Apply to all widgets
gtk_style_context_add_provider_for_display(display, 
    GTK_STYLE_PROVIDER(css_provider), 
    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

g_object_unref(css_provider);
```

### Method 2: Environment Variable
You can also load themes by setting an environment variable:

```bash
export ARIEL_THEME="themes/ocean.css"
./ariel
```

### Method 3: User Settings Directory
Copy your preferred theme to the user's config directory:

```bash
mkdir -p ~/.config/ariel
cp themes/forest.css ~/.config/ariel/theme.css
```

## Theme Structure

Each theme file targets specific GTK4 widgets and CSS classes used in Ariel:

- **window** - Main application background
- **headerbar** - Top header bar styling
- **button.suggested-action** - Primary action buttons
- **button.destructive-action** - Warning/delete buttons
- **.plugin-list** - LV2 plugin browser styling
- **.plugin-item:selected** - Selected plugin highlighting
- **.mixer-channel** - Audio mixer channel styling
- **scale** - Volume sliders and controls
- **entry** - Text input fields
- **progressbar** - Progress indicators

## Creating Custom Themes

To create your own theme:

1. Copy an existing theme file as a starting point
2. Modify the color values to match your preferred palette
3. Test the theme by loading it in Ariel
4. Save your custom theme with a descriptive name

### Color Guidelines

- Use hex color codes (#rrggbb format)
- Ensure sufficient contrast for text readability
- Test both light and dark variants if needed
- Consider colorblind accessibility when choosing colors

## Theme Integration in Ariel

To integrate theme support into Ariel:

1. Add theme selection to application preferences
2. Implement CSS provider loading in the main application
3. Add theme switching without restart capability
4. Store user's theme preference in GSettings or config file

## Contributing Themes

To contribute new themes:

1. Follow the existing naming convention
2. Test thoroughly with all UI components
3. Ensure accessibility compliance
4. Add your theme to this README
5. Submit a pull request

## License

These themes are distributed under the same license as the Ariel application.