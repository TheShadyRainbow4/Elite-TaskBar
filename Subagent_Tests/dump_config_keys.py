import re

with open('Win32Explorer_26.0.3.0/App_Source/ConfigRegistryStorage.cpp', 'r', encoding='utf-8') as f:
    for line in f:
        # Search for registry read/write functions or string literals
        if 'RegQueryValueEx' in line or 'RegSetValueEx' in line or 'L\"' in line:
            # If line contains L"something" let's print it if it looks like a setting name
            match = re.search(r'L"([a-zA-Z0-9_]{3,})"', line)
            if match:
                print(line.strip())
