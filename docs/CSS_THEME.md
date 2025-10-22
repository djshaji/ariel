# Ariel CSS Theme Documentation

## Overview
Ariel includes a beautiful soft pastel color scheme CSS theme that automatically applies to the GTK4 interface. The theme provides a modern, gentle aesthetic with carefully chosen colors and smooth animations.

## Color Palette

### Primary Colors
- **Pastel Pink** (`#f8d7da`) - For destructive actions (delete, remove)
- **Pastel Blue** (`#d4e8fc`) - For suggested actions (primary buttons)
- **Pastel Green** (`#d1ecf1`) - For success states and toggles
- **Pastel Purple** (`#e2d9f3`) - For headers and important elements
- **Pastel Yellow** (`#fff3cd`) - For warnings and highlights
- **Pastel Orange** (`#f7e5d3`) - For secondary accents
- **Pastel Gray** (`#f1f3f4`) - For backgrounds and borders

### Surface Colors
- **Surface 1** (`#fefefe`) - Main background
- **Surface 2** (`#f8f9fa`) - Secondary background
- **Surface 3** (`#e9ecef`) - Tertiary background

## CSS Classes Used in Ariel

### Typography
- `.title-1` - Largest headers (2.2em, purple)
- `.title-2` - Section headers (1.8em, blue)
- `.title-3` - Subsection headers (1.5em, green)
- `.title-4` - Component headers (1.2em, purple)
- `.title-5` - Small headers (1.1em, blue)
- `.caption` - Parameter labels (0.9em, secondary text)
- `.dim-label` - Disabled/empty state text (muted, italic)

### Buttons
- `.suggested-action` - Primary action buttons (blue gradient)
- `.destructive-action` - Delete/remove buttons (pink gradient)
- `.pill` - Rounded button style (border-radius: 20px)
- `.circular` - Circular buttons for transport controls (50% border-radius)

### Containers
- `.card` - Plugin panels and containers (rounded, shadowed)
- `.drop-target` - Drag and drop areas (dashed border, green gradient)

### Status Indicators
- `.status-online` - Connected/active state (green)
- `.status-offline` - Disconnected/inactive state (pink)

## Automatic Loading

The theme automatically loads in this order:

1. **System Theme** - GTK4 default styling
2. **Ariel Theme** - Our custom pastel theme (`APPLICATION` priority)
3. **User Custom CSS** - Personal overrides (`USER` priority)

### Development
In development mode, the theme loads from:
```
data/ariel-theme.css
```

### Production
After installation, the theme loads from:
```
/usr/share/ariel/ariel-theme.css
/usr/local/share/ariel/ariel-theme.css
```

## Custom User Styling

Users can override any styles by creating:
```
~/.config/ariel/style.css
```

This file will be loaded with the highest priority, allowing complete customization.

### Example Custom CSS
```css
/* Make suggested actions more vibrant */
.suggested-action {
  background: linear-gradient(135deg, #4a90e2, #667eea);
  color: white;
}

/* Darker card backgrounds */
.card {
  background: #f0f0f0;
  border: 2px solid #ddd;
}

/* Custom parameter label color */
.caption {
  color: #666;
  font-weight: 600;
}
```

## Features

### Modern GTK4 Support
- Proper CSS variable usage with `:root`
- Modern selectors and properties
- GTK4-specific styling for new widgets

### Accessibility
- High contrast mode support via `@media (prefers-contrast: high)`
- Reduced motion support via `@media (prefers-reduced-motion: reduce)`
- Proper focus indicators with outline styles
- Sufficient color contrast ratios

### Responsive Design
- Consistent padding and margins
- Scalable typography using em units
- Flexible layouts that adapt to content

### Smooth Interactions
- CSS transitions for hover states (0.2s ease)
- Subtle animations for cards and buttons
- Transform effects for interactive feedback

## Component-Specific Styling

### Parameter Controls
- **Scales**: Gradient track with circular slider
- **Toggle Buttons**: State-dependent coloring
- **File Choosers**: Prominent styling with emoji support

### Transport Controls
- **Circular buttons** for play/stop/record
- **State-dependent colors** (play=blue, record=red)
- **Smooth hover animations**

### Plugin Management
- **Card-based layout** for plugin containers
- **Drop target styling** for drag and drop
- **Hover effects** with elevation changes

## Browser/Runtime Support
- **GTK 4.0+** - Full support
- **Modern CSS features** - Variables, gradients, transforms
- **Fallback support** - Graceful degradation for older GTK versions

## Development Notes

The CSS follows modern best practices:
- BEM-like naming conventions
- Semantic color usage
- Modular component styling
- Performance-optimized selectors

All colors are carefully chosen for:
- Visual harmony
- Accessibility compliance
- Professional appearance
- Reduced eye strain during long sessions