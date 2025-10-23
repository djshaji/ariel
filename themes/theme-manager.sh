#!/bin/bash

# Ariel Theme Manager
# A utility script to preview and apply themes for the Ariel LV2 host

THEMES_DIR="$(dirname "$0")"
CONFIG_DIR="$HOME/.config/ariel"
CURRENT_THEME_FILE="$CONFIG_DIR/current-theme.css"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Create config directory if it doesn't exist
mkdir -p "$CONFIG_DIR"

show_help() {
    echo -e "${BLUE}Ariel Theme Manager${NC}"
    echo ""
    echo "Usage: $0 [COMMAND] [THEME_NAME]"
    echo ""
    echo "Commands:"
    echo -e "  ${GREEN}list${NC}           - List all available themes"
    echo -e "  ${GREEN}apply${NC} <theme>   - Apply a theme (copies to config directory)"
    echo -e "  ${GREEN}preview${NC} <theme> - Show theme colors and description"
    echo -e "  ${GREEN}current${NC}         - Show currently applied theme"
    echo -e "  ${GREEN}reset${NC}           - Remove current theme (use default)"
    echo -e "  ${GREEN}help${NC}            - Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 list"
    echo "  $0 apply teal"
    echo "  $0 preview dark"
    echo ""
}

list_themes() {
    echo -e "${BLUE}Available Ariel Themes:${NC}"
    echo ""
    
    # Primary color themes
    echo -e "${PURPLE}Primary Color Themes:${NC}"
    for theme in teal violet sunset ocean forest midnight rose gold lavender mint; do
        if [ -f "$THEMES_DIR/$theme.css" ]; then
            echo "  • $theme"
        fi
    done
    
    echo ""
    echo -e "${PURPLE}Special Themes:${NC}"
    for theme in soft-pink coral sky-blue dark; do
        if [ -f "$THEMES_DIR/$theme.css" ]; then
            echo "  • $theme"
        fi
    done
    
    echo ""
    echo -e "${YELLOW}Use '$0 preview <theme>' to see colors and details${NC}"
}

get_theme_color() {
    local theme_file="$1"
    # Extract primary color from CSS comment
    grep -o "#[0-9a-fA-F]\{6\}" "$theme_file" | head -1
}

preview_theme() {
    local theme_name="$1"
    local theme_file="$THEMES_DIR/$theme_name.css"
    
    if [ ! -f "$theme_file" ]; then
        echo -e "${RED}Error: Theme '$theme_name' not found${NC}"
        return 1
    fi
    
    echo -e "${BLUE}Theme Preview: $theme_name${NC}"
    echo ""
    
    # Extract and show primary color
    local primary_color=$(get_theme_color "$theme_file")
    echo -e "Primary Color: ${CYAN}$primary_color${NC}"
    
    # Show theme description from CSS comments
    local description=$(grep "Primary color:" "$theme_file" | sed 's/.*Primary color: //')
    if [ -n "$description" ]; then
        echo -e "Description: $description"
    fi
    
    echo ""
    echo "Theme features:"
    echo "  • Custom header bar styling"
    echo "  • Plugin list highlighting"
    echo "  • Mixer channel styling"
    echo "  • Button and control theming"
    echo "  • Volume slider customization"
    
    echo ""
    echo -e "${YELLOW}To apply this theme: $0 apply $theme_name${NC}"
}

apply_theme() {
    local theme_name="$1"
    local theme_file="$THEMES_DIR/$theme_name.css"
    
    if [ ! -f "$theme_file" ]; then
        echo -e "${RED}Error: Theme '$theme_name' not found${NC}"
        echo -e "Use '$0 list' to see available themes"
        return 1
    fi
    
    # Copy theme to config directory
    cp "$theme_file" "$CURRENT_THEME_FILE"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Theme '$theme_name' applied successfully${NC}"
        echo ""
        echo "The theme will be active the next time you start Ariel."
        echo -e "Current theme file: ${CYAN}$CURRENT_THEME_FILE${NC}"
        
        # Store theme name for reference
        echo "$theme_name" > "$CONFIG_DIR/theme-name.txt"
    else
        echo -e "${RED}Error: Failed to apply theme${NC}"
        return 1
    fi
}

show_current() {
    if [ -f "$CURRENT_THEME_FILE" ]; then
        local theme_name=""
        if [ -f "$CONFIG_DIR/theme-name.txt" ]; then
            theme_name=$(cat "$CONFIG_DIR/theme-name.txt")
        fi
        
        echo -e "${GREEN}Current theme:${NC} ${theme_name:-Unknown}"
        echo -e "Theme file: ${CYAN}$CURRENT_THEME_FILE${NC}"
        
        # Show primary color if available
        local primary_color=$(get_theme_color "$CURRENT_THEME_FILE")
        if [ -n "$primary_color" ]; then
            echo -e "Primary color: ${CYAN}$primary_color${NC}"
        fi
    else
        echo -e "${YELLOW}No custom theme applied (using default)${NC}"
    fi
}

reset_theme() {
    if [ -f "$CURRENT_THEME_FILE" ]; then
        rm "$CURRENT_THEME_FILE"
        rm -f "$CONFIG_DIR/theme-name.txt"
        echo -e "${GREEN}✓ Theme reset to default${NC}"
    else
        echo -e "${YELLOW}No custom theme to reset${NC}"
    fi
}

# Main script logic
case "$1" in
    "list"|"ls")
        list_themes
        ;;
    "apply")
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Please specify a theme name${NC}"
            echo "Usage: $0 apply <theme_name>"
            exit 1
        fi
        apply_theme "$2"
        ;;
    "preview"|"show")
        if [ -z "$2" ]; then
            echo -e "${RED}Error: Please specify a theme name${NC}"
            echo "Usage: $0 preview <theme_name>"
            exit 1
        fi
        preview_theme "$2"
        ;;
    "current"|"status")
        show_current
        ;;
    "reset"|"default")
        reset_theme
        ;;
    "help"|"-h"|"--help")
        show_help
        ;;
    "")
        show_help
        ;;
    *)
        echo -e "${RED}Error: Unknown command '$1'${NC}"
        echo ""
        show_help
        exit 1
        ;;
esac