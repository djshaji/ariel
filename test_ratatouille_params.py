#!/usr/bin/env python3
"""
Test script to verify Ratatouille plugin parameter detection
"""

import subprocess
import sys

def run_command(cmd):
    """Run a command and return its output"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        return result.stdout, result.stderr, result.returncode
    except Exception as e:
        return "", str(e), 1

def test_ratatouille_parameters():
    """Test Ratatouille parameter detection"""
    print("Testing Ratatouille LV2 plugin parameter detection...")
    
    # Get Ratatouille plugin URI
    stdout, stderr, code = run_command("lv2ls | grep -i ratatouille")
    if not stdout:
        print("❌ Ratatouille plugin not found")
        return
    
    uri = stdout.strip().split('\n')[0]
    print(f"✅ Found Ratatouille URI: {uri}")
    
    # Check for file parameters using lv2info
    print("\n=== Checking for patch:writable parameters ===")
    stdout, stderr, code = run_command(f"lv2info '{uri}' | grep -A 5 -B 5 'patch:writable'")
    if stdout:
        print("Found patch:writable declarations:")
        print(stdout)
    else:
        print("❌ No patch:writable parameters found")
    
    # Check for atom:Path ranges
    print("\n=== Checking for atom:Path ranges ===")  
    stdout, stderr, code = run_command(f"lv2info '{uri}' | grep -A 5 -B 5 'atom:Path'")
    if stdout:
        print("Found atom:Path declarations:")
        print(stdout)
    else:
        print("❌ No atom:Path ranges found")
        
    # Check for rdfs:label of parameters
    print("\n=== Checking parameter labels ===")
    stdout, stderr, code = run_command(f"lv2info '{uri}' | grep -A 2 -B 2 'rdfs:label'")
    if stdout:
        print("Found parameter labels:")
        print(stdout)
        
    # Count expected parameters
    expected_params = ["Neural Model A", "Neural Model B", "IR File", "IR File 1"]
    print(f"\n=== Expected Parameters ===")
    for param in expected_params:
        print(f"- {param}")
    
    print(f"\n✅ Should detect {len(expected_params)} file parameter controls")
    
    # Check Atom ports
    print("\n=== Checking Atom ports ===")
    stdout, stderr, code = run_command(f"lv2info '{uri}' | grep -A 5 -B 5 'AtomPort'")
    if stdout:
        print("Found Atom ports:")
        print(stdout)

if __name__ == "__main__":
    test_ratatouille_parameters()