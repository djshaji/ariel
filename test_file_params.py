#!/usr/bin/env python3
"""
Test script to verify file parameter detection in LV2 plugins
"""

import subprocess
import sys
import re

def run_command(cmd):
    """Run a command and return its output"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=10)
        return result.stdout, result.stderr, result.returncode
    except subprocess.TimeoutExpired:
        return "", "Command timed out", 1

def test_lv2_plugins():
    """Test LV2 plugin parameter detection"""
    print("Testing LV2 plugin file parameter support...")
    
    # Test Neural Amp Modeler
    print("\n=== Neural Amp Modeler ===")
    stdout, stderr, code = run_command("lv2ls | grep -i neural")
    if stdout:
        uri = stdout.strip().split('\n')[0]
        print(f"Found URI: {uri}")
        
        # Check parameters
        stdout, stderr, code = run_command(f"lv2info '{uri}' | grep -A 5 -B 5 'atom:Path\\|model'")
        if stdout:
            print("Parameters with atom:Path:")
            print(stdout)
        else:
            print("No atom:Path parameters found")
    
    # Test Ratatouille
    print("\n=== Ratatouille ===")
    stdout, stderr, code = run_command("lv2ls | grep -i ratatouille")
    if stdout:
        uri = stdout.strip().split('\n')[0]
        print(f"Found URI: {uri}")
        
        # Check parameters
        stdout, stderr, code = run_command(f"lv2info '{uri}' | grep -A 5 -B 5 'atom:Path\\|file'")
        if stdout:
            print("Parameters with atom:Path:")
            print(stdout)
        else:
            print("No atom:Path parameters found")
    
    # Test what our application detects
    print("\n=== Testing Ariel's Detection ===")
    print("Run Ariel and check the console output for:")
    print("1. Plugin loading messages")
    print("2. File parameter support detection")
    print("3. File dialog success/failure messages")

if __name__ == "__main__":
    test_lv2_plugins()