#include "ariel.h"
#include <stdio.h>

// Test to check ArielWindow structure layout and memory access
void test_window_layout() {
    printf("=== ArielWindow Structure Layout Test ===\n");
    
    // Print structure size and field offsets
    printf("sizeof(ArielWindow) = %zu bytes\n", sizeof(ArielWindow));
    printf("sizeof(GtkApplicationWindow) = %zu bytes\n", sizeof(GtkApplicationWindow));
    
    ArielWindow test_window = {0};
    
    printf("ArielWindow field offsets:\n");
    printf("  app field offset: %zu bytes\n", (char*)&test_window.app - (char*)&test_window);
    printf("  header_bar offset: %zu bytes\n", (char*)&test_window.header_bar - (char*)&test_window);
    printf("  audio_toggle offset: %zu bytes\n", (char*)&test_window.audio_toggle - (char*)&test_window);
    printf("  main_paned offset: %zu bytes\n", (char*)&test_window.main_paned - (char*)&test_window);
    
    // Test pointer storage and retrieval
    ArielApp *test_app = (ArielApp*)0x12345678ABCDEF00ULL;
    test_window.app = test_app;
    
    printf("\nPointer storage test:\n");
    printf("  Stored app pointer: %p\n", (void*)test_app);
    printf("  Retrieved app pointer: %p\n", (void*)test_window.app);
    printf("  Pointers match: %s\n", (test_window.app == test_app) ? "YES" : "NO");
}

int main() {
    test_window_layout();
    return 0;
}