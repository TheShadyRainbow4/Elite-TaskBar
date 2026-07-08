import re

with open('Win32Explorer_26.0.3.0/App_Source/Win32Explorer.rc', 'r', encoding='utf-8') as f:
    content = f.read()

# Find dialog blocks
# Dialog starts with IDD_XYZ DIALOGEX ... and ends with END
dialogs = ['IDD_OPTIONS_GENERAL', 'IDD_OPTIONS_FILES_FOLDERS', 'IDD_OPTIONS_ADVANCED', 'IDD_OPTIONS_APPEARANCE']
for dlg in dialogs:
    pattern = r'(' + dlg + r'\s+DIALOGEX.*?^END)'
    match = re.search(pattern, content, re.DOTALL | re.MULTILINE)
    if match:
        print(f"=== {dlg} ===")
        print(match.group(1))
        print("\n")
    else:
        print(f"=== {dlg} NOT FOUND ===")
