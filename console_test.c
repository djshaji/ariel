#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <debugapi.h>
#include <wincon.h>
#endif

int main(int argc, char *argv[])
{
    printf("=== CONSOLE OUTPUT TEST ===\n");
    
#ifdef _WIN32
    printf("Running on Windows platform\n");
    
    BOOL console_allocated = FALSE;
    
    // Try to allocate or attach to console
    if (AllocConsole()) {
        console_allocated = TRUE;
        printf("New console allocated\n");
    } else if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        console_allocated = TRUE;
        printf("Attached to parent console\n");
    } else {
        HWND console_window = GetConsoleWindow();
        if (console_window != NULL) {
            console_allocated = TRUE;
            printf("Using existing console\n");
        }
    }
    
    if (console_allocated) {
        // Redirect streams
        FILE *fp_out, *fp_err, *fp_in;
        freopen_s(&fp_out, "CONOUT$", "w", stdout);
        freopen_s(&fp_err, "CONOUT$", "w", stderr);
        freopen_s(&fp_in, "CONIN$", "r", stdin);
        
        SetConsoleTitle(TEXT("Console Output Test"));
        
        printf("Console redirection successful!\n");
        fprintf(stderr, "Error output test: stderr working\n");
        
        // Test various output methods
        printf("Testing printf: %s\n", "SUCCESS");
        fprintf(stdout, "Testing fprintf to stdout: %s\n", "SUCCESS");
        fprintf(stderr, "Testing fprintf to stderr: %s\n", "SUCCESS");
        
        fflush(stdout);
        fflush(stderr);
        
        printf("\nConsole test completed successfully!\n");
        printf("Press Enter to exit...\n");
        getchar();
    } else {
        OutputDebugStringA("Console test: Failed to allocate/attach console\n");
        printf("Failed to setup console output\n");
    }
    
#else
    printf("Running on non-Windows platform\n");
    printf("Console output should work normally\n");
#endif
    
    printf("=== TEST COMPLETED ===\n");
    return 0;
}